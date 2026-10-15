#include <vector>

#include "beat/BeatEngine.h"
#include "audio/AudioCapture.h"

using namespace std;

BeatDetektor *det_low;
BeatDetektor *det_high;
BeatDetektor *det;
BeatDetektorVU *vu;
BeatDetektorContest *contest;

static vector<float> fft_collapse;

// ---- Beat detection ----

// Create the low/high-range detectors, VU follower, and BPM contest.
void initBD() {
    det_low = new BeatDetektor(90,160);
    det_high = new BeatDetektor(140, 260);

    det = det_low;

    vu = new BeatDetektorVU();

    contest = new BeatDetektorContest();
    contest->no_contest_decay = false;
    contest->finish_line = 1000;
}

// Feed both detectors the low/high 256-sample FFT windows, run the contest
// on the selected range, and update VU levels from the winning BPM.
void processBD(float timer_seconds, float dt) {
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

