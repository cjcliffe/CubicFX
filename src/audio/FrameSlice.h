#pragma once

// Per-frame audio slicing for the visual sample buffer.
//
// A full BUF-sized block at 44.1 kHz spans ~23 ms, so publishing only whole
// blocks refreshes the waveform at ~43 Hz.
// Instead the render thread drains newly arrived samples every frame into
// the rolling BUF-wide window, so the waveform stays live at the render
// rate while the detector below still assembles whole blocks at its own
// cadence. A frame with no fresh samples holds the display window still;
// the next real samples shift it forward again.
//
// Slices are disjoint and consecutive: every real sample enters the pipeline
// exactly once, in order. The rolling display window may hold past audio
// for smoothness, but the detector queue below only ever emits whole,
// correctly-timed blocks with no repeats and no invented samples.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

// Samples expected per slice: rate times duration (rounded, >= 1).
inline size_t expectedSliceSamples(unsigned sampleRate, float sliceSeconds) {
    if (sampleRate == 0 || !(sliceSeconds > 0.0f)) {
        return 1;
    }
    size_t n = static_cast<size_t>((double)sampleRate * (double)sliceSeconds + 0.5);
    return n == 0 ? 1 : n;
}

// Samples expected per 60 Hz render frame (convenience wrapper).
inline size_t expectedFrameSamples(unsigned sampleRate) {
    return expectedSliceSamples(sampleRate, 1.0f / 60.0f);
}

// Slice cadence: folds render-frame dt into steady audio slices. advance()
// returns 0 until a full period has accumulated, then the covered duration
// (which the slice consumes) while keeping the remainder, so a hitched frame
// yields one catch-up slice and the long-run rate holds.
struct SliceCadence {
    explicit SliceCadence(float slicesPerSecond = 30.0f)
        : period(slicesPerSecond > 0.0f ? 1.0f / slicesPerSecond : 1.0f / 30.0f) {}
    float advance(float dt) {
        if (!(dt > 0.0f) || dt > 0.25f) {
            dt = 1.0f / 60.0f;
        }
        accum += dt;
        if (accum < period) {
            return 0.0f;
        }
        float step = accum;
        accum -= period;
        return step;
    }
    void reset() { accum = 0.0f; }
    float period;
    float accum = 0.0f;
};

// Shift the rolling display window left by n and append src[0..n). Repairs a
// wrong-sized window to windowSize first. A slice >= windowSize keeps only
// its tail. An empty slice leaves the window still.
inline void appendSlice(std::vector<float> &window, size_t windowSize, const float *src, size_t n) {
    if (windowSize == 0) {
        return;
    }
    if (window.size() != windowSize) {
        window.assign(windowSize, 0.0f);
    }
    if (n == 0) {
        return;
    }
    if (src == nullptr) {
        return;
    }
    if (n >= windowSize) {
        window.assign(src + (n - windowSize), src + n);
        return;
    }
    window.erase(window.begin(), window.begin() + static_cast<std::ptrdiff_t>(n));
    window.insert(window.end(), src, src + n);
}

// Generate n samples of the 440 + 660 + 90 Hz stand-in tone, advancing phase
// by exactly n / sampleRate seconds so consecutive calls tile one seamless,
// correctly-timed timeline.
inline std::vector<float> synthesizeToneSamples(size_t n, double &phase, double sampleRate) {
    std::vector<float> tone;
    if (n == 0 || sampleRate <= 0.0) {
        return tone;
    }
    const float twoPi = 2.0f * 3.14159265f;
    tone.reserve(n);
    for (size_t i = 0; i < n; i++) {
        float t = static_cast<float>(phase + static_cast<double>(i) / sampleRate);
        tone.push_back(0.5f * sinf(twoPi * 440.0f * t)
            + 0.25f * sinf(twoPi * 660.0f * t)
            + 0.1f * sinf(twoPi * 90.0f * t));
    }
    phase += static_cast<double>(n) / sampleRate;
    if (phase > 3600.0) {
        phase -= 3600.0;
    }
    return tone;
}

// Append n tone samples to the rolling display window.
inline void synthesizeToneSlice(std::vector<float> &window, size_t windowSize,
    double &phase, double sampleRate, size_t n) {
    if (windowSize == 0 || n == 0 || sampleRate <= 0.0) {
        return;
    }
    if (window.size() != windowSize) {
        window.assign(windowSize, 0.0f);
    }
    std::vector<float> tone = synthesizeToneSamples(n, phase, sampleRate);
    appendSlice(window, windowSize, tone.data(), tone.size());
}

// Pop the oldest windowSize samples from the detector queue as one disjoint,
// correctly-timed block. Returns empty while fewer are banked; banked samples
// stay queued in order and each is emitted exactly once.
inline std::vector<float> popBlock(std::vector<float> &fifo, size_t windowSize) {
    std::vector<float> block;
    if (windowSize == 0 || fifo.size() < windowSize) {
        return block;
    }
    block.assign(fifo.begin(), fifo.begin() + static_cast<std::ptrdiff_t>(windowSize));
    fifo.erase(fifo.begin(), fifo.begin() + static_cast<std::ptrdiff_t>(windowSize));
    return block;
}
