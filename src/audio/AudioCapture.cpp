#define NOMINMAX
#define NOGDI
#define NOUSER

#include <cstdio>
#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <RtAudio.h>

#include "audio/AudioCapture.h"
#include "FFT.h"

using namespace std;

vector<float> sample_data;
vector<float> fft_data;
bool agcEnabled = true;
bool testToneEnabled = false;

unsigned int audioDefaultInputDevice() {
    return RtAudio().getDefaultInputDevice();
}

unsigned int audioDefaultOutputDevice() {
    return RtAudio().getDefaultOutputDevice();
}

// ---- Audio capture ----
// RtAudio delivers input on its own callback thread; the render thread picks
// up complete blocks. staging accumulates callback samples, readyBlock holds
// the latest full block, blockFresh flags it, all guarded by audioMutex.
static std::unique_ptr<RtAudio> audioIn;
static unsigned int audioChannels = 0;
static vector<float> staging;
static vector<float> readyBlock;
static bool blockFresh = false;
static bool audioRunning = false;
static mutex audioMutex;

// List every usable capture source, for the setup dialog. Physical inputs
// expose inputChannels; render endpoints expose only outputChannels and
// are offered as loopback ("Desktop Audio") sources: RtAudio's WASAPI
// backend captures them with AUDCLNT_STREAMFLAGS_LOOPBACK when a render
// device is opened as input.
vector<AudioDevInfo> listInputDevices() {
    vector<AudioDevInfo> devs;
    RtAudio probe;
    for (unsigned int id : probe.getDeviceIds()) {
        RtAudio::DeviceInfo info = probe.getDeviceInfo(id);
        if (info.inputChannels > 0) {
            devs.push_back({ id, info.name, info.inputChannels, info.outputChannels,
                false, info.isDefaultInput, info.isDefaultOutput });
        } else if (info.outputChannels > 0) {
            devs.push_back({ id, info.name, info.outputChannels, info.outputChannels,
                true, info.isDefaultInput, info.isDefaultOutput });
        }
    }
    return devs;
}

// RtAudio input callback (runs on the audio thread). Mixes to mono and
// accumulates into staging; each full BUF-sized block is published to
// readyBlock. Only locking and copying here, no allocation or prints.
static int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames,
    double streamTime, RtAudioStreamStatus status, void *userData) {
    if (status) {
        printf("Audio stream overflow.\n");
    }
    if (inputBuffer == nullptr) {
        return 0;
    }
    float *input = (float *)inputBuffer;
    lock_guard<mutex> lock(audioMutex);
    for (unsigned int i = 0; i < nFrames; i++) {
        float mono = 0.0f;
        for (unsigned int c = 0; c < audioChannels; c++) {
            mono += input[i * audioChannels + c];
        }
        mono /= (float)audioChannels;
        staging.push_back(mono);
    }
    while (staging.size() >= BUF) {
        readyBlock.assign(staging.begin(), staging.begin() + BUF);
        staging.erase(staging.begin(), staging.begin() + BUF);
        blockFresh = true;
        if (staging.size() > BUF) {
            staging.erase(staging.begin(), staging.end() - BUF);
        }
    }
    return 0;
}

// Open a float32 input stream on deviceId (stereo when the device offers 2+
// channels, else mono), preferring SRATE and falling back to the device's
// first supported rate. Render endpoints (no input channels, output
// channels only) are opened as WASAPI loopback sources; RtAudio enables
// AUDCLNT_STREAMFLAGS_LOOPBACK for them. Returns false if the device
// can't provide input or loopback.
bool openAudio(unsigned int deviceId) {
    audioIn = std::make_unique<RtAudio>();
    RtAudio::DeviceInfo info = audioIn->getDeviceInfo(deviceId);
    bool loopback = (info.inputChannels == 0 && info.outputChannels > 0);
    if (!loopback && info.inputChannels == 0) {
        printf("Selected device has no input channels.\n");
        audioIn.reset();
        return false;
    }
    unsigned int sourceChannels = loopback ? info.outputChannels : info.inputChannels;
    audioChannels = sourceChannels >= 2 ? 2 : 1;

    unsigned int sampleRate = SRATE;
    bool rateOk = false;
    for (unsigned int r : info.sampleRates) {
        if (r == SRATE) {
            rateOk = true;
            break;
        }
    }
    if (!rateOk && !info.sampleRates.empty()) {
        sampleRate = info.sampleRates[0];
        printf("Device does not support %d Hz, using %u Hz.\n", SRATE, sampleRate);
    }

    RtAudio::StreamParameters params;
    params.deviceId = deviceId;
    params.nChannels = audioChannels;
    params.firstChannel = 0;
    unsigned int bufferFrames = BUF;

    RtAudioErrorType err = audioIn->openStream(nullptr, &params, RTAUDIO_FLOAT32,
        sampleRate, &bufferFrames, &audioCallback);
    if (err != RTAUDIO_NO_ERROR) {
        printf("Could not open audio stream: %s\n", audioIn->getErrorText().c_str());
        audioIn.reset();
        return false;
    }
    err = audioIn->startStream();
    if (err != RTAUDIO_NO_ERROR) {
        printf("Could not start audio stream: %s\n", audioIn->getErrorText().c_str());
        audioIn->closeStream();
        audioIn.reset();
        return false;
    }

    sample_data.assign(BUF, 0.0f);
    fft_data.assign(BUF, 0.0f);
    readyBlock.assign(BUF, 0.0f);
    audioRunning = true;
    if (loopback) {
        printf("Opened '%s' as desktop-audio loopback (%u ch).\n", info.name.c_str(), audioChannels);
    }
    return true;
}

// Stop and release the audio stream; safe to call with no stream open.
void closeAudio() {
    if (audioIn != nullptr) {
        if (audioIn->isStreamRunning()) {
            audioIn->stopStream();
        }
        if (audioIn->isStreamOpen()) {
            audioIn->closeStream();
        }
        audioIn.reset();
    }
    audioRunning = false;
}

static float normGain = 1.0f;
static double tonePhase = 0.0;

void audioResetSession(bool agc, bool testTone) {
    sample_data.assign(BUF, 0.0f);
    fft_data.assign(BUF, 0.0f);
    normGain = 1.0f;
    agcEnabled = agc;
    testToneEnabled = testTone;
    lock_guard<mutex> lock(audioMutex);
    staging.clear();
    blockFresh = false;
}

// Automatic gain: strip DC, steer the block peak toward 0.9 with a smoothed
// gain (fast attack 0.4, slow release 0.03, clamped to 0.5x-32x so silence
// freezes the gain instead of pumping noise), then soft-clip with tanh.
static void normalizeAudioBlock(vector<float> &block) {
    double mean = 0.0;
    for (float v : block) {
        mean += v;
    }
    mean /= (double)block.size();

    float peak = 0.0f;
    for (float v : block) {
        float a = fabsf((float)(v - mean));
        if (a > peak) {
            peak = a;
        }
    }

    if (peak > 1e-4f) {
        float target = 0.9f / peak;
        if (target > 32.0f) {
            target = 32.0f;
        }
        if (target < 0.5f) {
            target = 0.5f;
        }
        float rate = (target > normGain) ? 0.4f : 0.03f;
        normGain += (target - normGain) * rate;
    }

    for (float &v : block) {
        v = tanhf(normGain * (float)(v - mean));
    }
}

// Stand-in input: 440 + 660 + 90 Hz mix at a healthy level, so shaders can
// be exercised with no microphone. Phase wraps hourly on exact cycle
// boundaries to stay click-free.
static void synthesizeTestTone() {
    const float twoPi = 2.0f * 3.14159265f;
    sample_data.assign(BUF, 0.0f);
    for (int i = 0; i < BUF; i++) {
        float t = (float)(tonePhase + (double)i / (double)SRATE);
        sample_data[i] = 0.5f * sinf(twoPi * 440.0f * t)
            + 0.25f * sinf(twoPi * 660.0f * t)
            + 0.1f * sinf(twoPi * 90.0f * t);
    }
    tonePhase += (double)BUF / (double)SRATE;
    if (tonePhase > 3600.0) {
        tonePhase -= 3600.0;
    }
}

// Per-frame audio step: test tone, or the latest captured block (optional
// gain), then in-place FFT into fft_data for the shaders and BeatDetektor.
// Skips silently when no fresh block arrived this frame.
void captureAudio() {
    if (testToneEnabled) {
        synthesizeTestTone();
    } else {
        if (!audioRunning) {
            return;
        }
        bool fresh = false;
        {
            lock_guard<mutex> lock(audioMutex);
            if (blockFresh) {
                sample_data = readyBlock;
                blockFresh = false;
                fresh = true;
            }
        }
        if (!fresh) {
            return;
        }
        if (agcEnabled) {
            normalizeAudioBlock(sample_data);
        }
    }

    fft_data = sample_data;

    DanielsonLanczos <(BUF/2), float> dlfft;

    unsigned long n, m, j, i;

    n = (BUF/2)<<1;
    j=1;
    for (i=1; i<n; i+=2) {
        if (j>i) {
            swap(fft_data[j-1], fft_data[i-1]);
            swap(fft_data[j], fft_data[i]);
        }
        m = (BUF/2);
        while (m>=2 && j>m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    };

    dlfft.apply(&fft_data[0]);
}

