#define NOMINMAX
#define NOGDI
#define NOUSER

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <RtAudio.h>

#include "audio/AudioCapture.h"
#include "audio/FrameSlice.h"
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
// RtAudio delivers input on its own callback thread; the render thread drains
// staging every frame into the display window and detector queue (see
// FrameSlice.h); all shared state guarded by audioMutex.
static std::unique_ptr<RtAudio> audioIn;
static unsigned int audioChannels = 0;
static unsigned int audioRate = SRATE;
static vector<float> staging;
static vector<float> rawWindow;
// Detector queue: banked real samples awaiting whole-block emission. Each
// sample below enters exactly once, in order; only whole, correctly-timed
// blocks reach the detector, never repeats, overlaps, or invented samples.
static vector<float> bdFifo;
static bool audioRunning = false;
// Slice clock: display slices run every render frame (~60 Hz) while the
// detector below keeps its whole-block cadence.
static SliceCadence sliceClock(60.0f);
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
// accumulates into staging for the render thread to drain each frame.
// Only locking and copying here, no allocation or prints.
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
    // Bound staging when the render thread stalls; the per-frame drain takes
    // whatever has arrived, so nothing is published here.
    if (staging.size() > MAXBANK) {
        staging.erase(staging.begin(), staging.end() - MAXBANK);
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
    unsigned int bufferFrames = CBFRAMES;

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

    printf("Audio stream: %u Hz, %u-frame callback buffer.\n", sampleRate, bufferFrames);
    audioRate = sampleRate;
    bdFifo.clear();
    sample_data.assign(BUF, 0.0f);
    fft_data.assign(BUF, 0.0f);
    rawWindow.assign(BUF, 0.0f);
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
    sliceClock.reset();
    bdFifo.clear();
    rawWindow.assign(BUF, 0.0f);
}

// Automatic gain: strip DC, steer the block peak toward 0.9 with a smoothed
// gain (fast attack 0.4, slow release 0.03, clamped to 0.5x-32x so silence
// freezes the gain instead of pumping noise), then soft-clip with tanh.
// Attack/release were tuned per whole-block update; the caller passes the
// block's own duration so the per-block tuning holds at any block size.
static void normalizeAudioBlock(vector<float> &block, float dt) {
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
        float step = dt / ((float)BUF / (float)SRATE);
        float attack = 1.0f - powf(1.0f - 0.4f, step);
        float release = 1.0f - powf(1.0f - 0.03f, step);
        float rate = (target > normGain) ? attack : release;
        normGain += (target - normGain) * rate;
    }

    for (float &v : block) {
        v = tanhf(normGain * (float)(v - mean));
    }
}

// Display gain: same DC-strip and soft-clip curve as normalizeAudioBlock but
// reusing the current gain without adapting it, so the visual window matches
// the detector path while each sample steers the gain exactly once.
static void applyDisplayGain(vector<float> &block) {
    double mean = 0.0;
    for (float v : block) {
        mean += v;
    }
    mean /= (double)block.size();
    for (float &v : block) {
        v = tanhf(normGain * (float)(v - mean));
    }
}

// Audio slice step (each render frame): collect newly arrived samples,
// shift the rolling display window forward, and bank the samples for
// the detector. Returns true when a fresh, correctly-timed detector
// block was published to fft_data: the oldest banked BUF samples, each
// consumed exactly once, in order, with no repeats, overlaps, or
// invented samples. A starved frame holds the display window still and
// banks nothing. Frames without a fresh block reuse the latest fft_data.
bool captureAudio(float dt) {
    if (!testToneEnabled && !audioRunning) {
        sliceClock.reset();
        return false;
    }
    // Display refreshes every frame; detector blocks complete less often.
    // (dt is sanitized inside SliceCadence::advance.)
    float step = sliceClock.advance(dt);
    if (!(step > 0.0f)) {
        return false;
    }
    // This slice's real samples: disjoint, consecutive, each consumed exactly
    // once, in order. The display window below keeps part of the last slice's
    // waveform; the detector queue only ever emits whole, correctly-timed
    // blocks.
    vector<float> fresh;
    if (testToneEnabled) {
        size_t n = (size_t)((double)SRATE * (double)step + 0.5);
        n = max<size_t>(1, min<size_t>(n, (size_t)MAXBANK));
        fresh = synthesizeToneSamples(n, tonePhase, (double)SRATE);
    } else {
        lock_guard<mutex> lock(audioMutex);
        fresh.swap(staging);
    }
    // Display only: advance by what really arrived. Starved slices hold the
    // window still instead of stretching invented samples across it.
    if (!fresh.empty()) {
        appendSlice(rawWindow, BUF, fresh.data(), fresh.size());
    }
    sample_data = rawWindow;
    applyDisplayGain(sample_data);

    bdFifo.insert(bdFifo.end(), fresh.begin(), fresh.end());
    if (bdFifo.size() > (size_t)MAXBANK) {
        bdFifo.erase(bdFifo.begin(), bdFifo.end() - (size_t)MAXBANK);
    }
    vector<float> block = popBlock(bdFifo, BUF);
    if (block.empty()) {
        return false;
    }
    if (agcEnabled) {
        // Blocks are always exactly BUF samples, so scaling by the block's
        // own duration matches the original tuning 1:1 at the nominal rate.
        float blockSeconds = (float)BUF / (float)(testToneEnabled ? SRATE : audioRate);
        normalizeAudioBlock(block, blockSeconds);
    }
    fft_data = block;

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
    return true;
}
