#pragma once

#include <memory>

#include "BeatDetektor.h"

class BeatEngine {
public:
    BeatEngine();
    void process(float timer_seconds, float dt);

    std::unique_ptr<BeatDetektor> det_low;
    std::unique_ptr<BeatDetektor> det_high;
    BeatDetektor *det = nullptr;
    std::unique_ptr<BeatDetektorVU> vu;
    std::unique_ptr<BeatDetektorContest> contest;
};
