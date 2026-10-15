#pragma once

#include <vector>

#include "audio/AudioCapture.h"

inline constexpr int DIALOG_W = 660;
inline constexpr int DIALOG_H = 720;

struct SetupChoice {
    int monitor = 0;
    bool fullscreen = false;
    int width = 1280;
    int height = 768;
    int resPreset = -1;
    unsigned int audioDevice = 0;
    int audioIndex = 0;
    bool agc = true;
    bool testTone = false;
    bool vsync = true;
};

struct ResPreset {
    int width;
    int height;
};

bool runSetupDialog(SetupChoice &choice, std::vector<AudioDevInfo> &audioDevs);
