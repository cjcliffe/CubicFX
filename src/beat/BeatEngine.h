#pragma once

#include <memory>

#include "BeatDetektor.h"

class BeatEngine {
public:
    BeatEngine();
    // spectrumFresh marks a new correctly-timed detector block in fft_data;
    // detectors and contest run only then, VU smoothing runs every frame.
    void process(float timer_seconds, float dt, bool spectrumFresh);

    std::unique_ptr<BeatDetektor> det_low;
    std::unique_ptr<BeatDetektor> det_high;
    BeatDetektor *det = nullptr;
    std::unique_ptr<BeatDetektorVU> vu;
    std::unique_ptr<BeatDetektorContest> contest;
};
