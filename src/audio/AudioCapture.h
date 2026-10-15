#pragma once

#include <string>
#include <vector>

#define SRATE 44100
#define BUF 2048

struct AudioDevInfo {
    unsigned int id;
    std::string name;
    unsigned int inChannels;
};

extern std::vector<float> sample_data;
extern std::vector<float> fft_data;
extern bool agcEnabled;
extern bool testToneEnabled;

std::vector<AudioDevInfo> listInputDevices();
bool openAudio(unsigned int deviceId);
void closeAudio();
void captureAudio();
unsigned int audioDefaultInputDevice();
void audioResetSession(bool agc, bool testTone);
