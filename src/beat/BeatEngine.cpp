#include <vector>

#include "beat/BeatEngine.h"
#include "audio/AudioCapture.h"

using namespace std;

static vector<float> fft_collapse;

// Create the low/high-range detectors, VU follower, and BPM contest.
BeatEngine::BeatEngine() {
    det_low = std::make_unique<BeatDetektor>(90,160);
    det_high = std::make_unique<BeatDetektor>(140, 260);

    det = det_low.get();

    vu = std::make_unique<BeatDetektorVU>();

    contest = std::make_unique<BeatDetektorContest>();
    contest->no_contest_decay = false;
    contest->finish_line = 1000;
}

// Feed fresh spectra to the detectors and contest, and update VU levels
// from the winning BPM every frame. Detectors see each correctly-timed
// block exactly once: repeats or overlaps of the same audio latch their
// trigger averages (see BeatDetektor::process), so spectrum input stays
// gated on spectrumFresh while VU smoothing stays per-frame for display.
void BeatEngine::process(float timer_seconds, float dt, bool spectrumFresh) {
    if (spectrumFresh) {
        if (fft_collapse.size() != 512) {
            fft_collapse.resize(512);
        }
        for (int x = 0; x < 256; x++)
        {
            fft_collapse[x] = fft_data[x];
        }
        for (int x = BUF-256; x < BUF; x++)
        {
            fft_collapse[x - (BUF - 512)] = fft_data[x];
        }

        det_low->process(timer_seconds,fft_collapse);
        det_high->process(timer_seconds, fft_collapse);
        contest->process(timer_seconds, det);

        contest->run();
    }

    if (contest->win_bpm_int)
        vu->process(det,dt,((float)contest->win_bpm_int/10.0));
}
