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

// Feed both detectors the low/high 256-sample FFT windows, run the contest
// on the selected range, and update VU levels from the winning BPM.
void BeatEngine::process(float timer_seconds, float dt) {
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

    if (contest->win_bpm_int)
        vu->process(det,dt,((float)contest->win_bpm_int/10.0));

    contest->run();
}
