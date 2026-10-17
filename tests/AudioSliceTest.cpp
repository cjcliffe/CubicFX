#include <cmath>
#include <cstdio>
#include <vector>

#include "audio/FrameSlice.h"
#include "audio/AudioCapture.h"

// Audio slicing for the visual sample buffer: each render frame shifts a
// rolling window forward instead of whole-block steps; a starved frame
// holds the window still instead of inventing samples.

static int failures = 0;

static void check(bool ok, const char *name) {
    if (!ok) {
        ++failures;
        std::printf("FAIL %s\n", name);
    } else {
        std::printf("ok %s\n", name);
    }
}

static bool near(float a, float b, float eps = 1e-5f) {
    return fabsf(a - b) <= eps;
}

int main() {
    check(expectedFrameSamples(44100) == 735, "frame slice at 44.1k is 735");
    check(expectedFrameSamples(48000) == 800, "frame slice at 48k is 800");
    check(expectedFrameSamples(8000) == 133, "frame slice at 8k is 133");
    check(expectedFrameSamples(0) == 1, "frame slice never zero");

    {
        std::vector<float> window(8, 0.0f);
        float slice[3] = { 1.0f, 2.0f, 3.0f };
        appendSlice(window, 8, slice, 3);
        check(window.size() == 8, "append keeps window size");
        bool tail = near(window[5], 1.0f) && near(window[6], 2.0f) && near(window[7], 3.0f);
        bool head = near(window[0], 0.0f) && near(window[4], 0.0f);
        check(tail && head, "append shifts slice into tail");
    }
    {
        std::vector<float> window(8, 0.0f);
        float slice[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        appendSlice(window, 8, slice, 10);
        check(near(window[0], 2.0f) && near(window[7], 9.0f), "oversized slice keeps its tail");
    }
    {
        // A starved slice appends nothing, so the display holds the last
        // frame's waveform instead of stretching invented samples across it.
        std::vector<float> window(8, 1.0f);
        appendSlice(window, 8, nullptr, 0);
        check(near(window[0], 1.0f) && near(window[7], 1.0f), "starved slice holds the window still");
    }
    {
        // Consecutive slices each move the window; the old whole-block
        // handoff sat unchanged between block arrivals.
        std::vector<float> window(8, 0.0f);
        std::vector<float> prev = window;
        bool moved = true;
        for (int f = 1; f <= 3; f++) {
            float slice[3] = { (float)f, (float)f, (float)f };
            appendSlice(window, 8, slice, 3);
            moved = moved && (window != prev);
            prev = window;
        }
        check(moved, "every frame slice advances the window");
        check(near(window[7], 3.0f) && near(window[0], 1.0f), "slices land in order");
    }
    {
        // Two half slices join exactly like one full slice: tone phase stays
        // time-correct instead of running ahead of real time.
        std::vector<float> a(16, 0.0f), b(16, 0.0f);
        double pa = 0.0, pb = 0.0;
        synthesizeToneSlice(a, 16, pa, 44100.0, 8);
        synthesizeToneSlice(a, 16, pa, 44100.0, 8);
        synthesizeToneSlice(b, 16, pb, 44100.0, 16);
        bool joined = a.size() == b.size();
        for (size_t i = 0; joined && i < a.size(); i++) {
            joined = near(a[i], b[i], 1e-5f);
        }
        check(joined, "split tone slices join seamlessly");
        check(near((float)pa, 16.0f / 44100.0f, 1e-7f), "tone phase advances in real time");
        check(near(b[0], 0.0f), "tone starts at zero crossing");
    }

    check(expectedSliceSamples(44100, 1.0f / 30.0f) == 1470, "slice at 44.1k/30Hz is 1470");
    check(expectedSliceSamples(48000, 1.0f / 30.0f) == 1600, "slice at 48k/30Hz is 1600");
    check(expectedSliceSamples(0, 1.0f / 30.0f) == 1, "slice never zero");
    {
        // Slice cadence: ~30 Hz slices from faster render frames, so the
        // beat detector keeps a calm cadence while rendering stays smooth.
        SliceCadence cadence(30.0f);
        float first = cadence.advance(0.02f);
        float second = cadence.advance(0.02f);
        check(!(first > 0.0f) && second > 0.0f, "slice fires after a full period");
        check(near(second, 0.04f, 1e-6f), "slice covers the accumulated frames");
        check(!(cadence.advance(0.02f) > 0.0f), "cadence rests between slices");
    }
    {
        // A hitched frame still produces exactly one catch-up slice.
        SliceCadence cadence(30.0f);
        float step = cadence.advance(0.1f);
        check(step > 0.0f && near(step, 0.1f, 1e-6f), "hitch yields one full catch-up slice");
    }
    {
        SliceCadence cadence(30.0f);
        cadence.advance(0.02f);
        cadence.reset();
        check(!(cadence.advance(0.02f) > 0.0f), "reset restarts the cadence");
    }

    {
        // Detector queue: whole, disjoint, correctly-timed blocks in order.
        std::vector<float> fifo;
        float s1[3] = { 1.0f, 2.0f, 3.0f };
        float s2[3] = { 4.0f, 5.0f, 6.0f };
        fifo.insert(fifo.end(), s1, s1 + 3);
        check(popBlock(fifo, 4).empty(), "short bank stays queued");
        check(fifo.size() == 3, "short bank keeps every sample");
        fifo.insert(fifo.end(), s2, s2 + 3);
        std::vector<float> block = popBlock(fifo, 4);
        check(block.size() == 4 && block[0] == 1.0f && block[3] == 4.0f, "block pops oldest first");
        check(fifo.size() == 2 && fifo[0] == 5.0f && fifo[1] == 6.0f, "remainder stays queued in order");
        std::vector<float> rest = popBlock(fifo, 2);
        check(rest.size() == 2 && rest[0] == 5.0f && rest[1] == 6.0f, "queued samples emit in order");
        check(fifo.empty() && popBlock(fifo, 2).empty(), "every sample emitted exactly once");
    }
    {
        // Tone timeline: consecutive slices tile without repeat or gap, so
        // detector blocks assembled from them stay correctly timed.
        double pa = 0.0, pb = 0.0;
        std::vector<float> a1 = synthesizeToneSamples(8, pa, 44100.0);
        std::vector<float> a2 = synthesizeToneSamples(8, pa, 44100.0);
        std::vector<float> b = synthesizeToneSamples(16, pb, 44100.0);
        bool tiled = a1.size() == 8 && a2.size() == 8 && b.size() == 16;
        for (size_t i = 0; tiled && i < 8; i++) {
            tiled = near(a1[i], b[i], 1e-5f) && near(a2[i], b[i + 8], 1e-5f);
        }
        check(tiled, "tone slices tile without repeat or gap");
    }

    {
        // Display cadence: at 60 slices/sec every render frame advances the
        // waveform, while detector blocks still assemble whole below.
        SliceCadence cadence(60.0f);
        bool every = true;
        for (int i = 0; i < 4; i++) {
            every = cadence.advance(1.0f / 60.0f) > 0.0f && every;
        }
        check(every, "60Hz cadence fires every frame");
    }

    {
        // Callback granularity: chunks smaller than detector blocks and
        // no bigger than ~2 frames worth, so the waveform scrolls smoothly.
        check(CBFRAMES > 0 && CBFRAMES <= BUF, "callback chunks fit within a detector block");
        check(MAXBANK >= BUF, "safety valve holds whole blocks");
        check((size_t)CBFRAMES <= 2 * expectedFrameSamples(SRATE), "bursts cover at most two frames");
    }

    if (failures == 0) {
        std::printf("ALL PASS\n");
        return 0;
    }
    std::printf("%d FAILURES\n", failures);
    return 1;
}
