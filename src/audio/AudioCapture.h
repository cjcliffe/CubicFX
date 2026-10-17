#pragma once

#include <string>
#include <vector>

#define SRATE 44100
#define BUF 1024
// Audio callback granularity (~23 ms at 44.1 kHz): bursts stay within one
// detector block while keeping callback overhead down; the detector still
// assembles whole BUF blocks downstream. A hint only: the backend may
// grant another size (logged at open).
#define CBFRAMES 1024
// Safety valve: never bank more than this many samples anywhere (callback
// staging, detector queue, single tone slice); oldest is dropped first.
// Bounds memory when the render thread stalls.
#define MAXBANK (4 * BUF)

struct AudioDevInfo {
    unsigned int id;
    std::string name;
    unsigned int inChannels;
    unsigned int outChannels;
    bool isLoopback;
    bool isDefaultInput;
    bool isDefaultOutput;
};

extern std::vector<float> sample_data;
extern std::vector<float> fft_data;
extern bool agcEnabled;
extern bool testToneEnabled;

std::vector<AudioDevInfo> listInputDevices();
bool openAudio(unsigned int deviceId);
void closeAudio();
// Advance one audio slice; returns true when a fresh correctly-timed
// detector block was published to fft_data (and sample_data refreshed).
bool captureAudio(float dt);
unsigned int audioDefaultInputDevice();
unsigned int audioDefaultOutputDevice();
void audioResetSession(bool agc, bool testTone);
