#include <cstdio>

#include "viz/Transition.h"

// Transition state machine: normal timed handover plus the rapid
// back-and-forth retarget that used to freeze the fade halfway.

static int failures = 0;

static void check(bool ok, const char *name) {
    if (!ok) {
        ++failures;
        std::printf("FAIL %s\n", name);
    } else {
        std::printf("ok %s\n", name);
    }
}

struct DummyViz {
    int id;
};

int main() {
    DummyViz a{ 1 }, b{ 2 }, c{ 3 };
    {
        TransitionState s;
        DummyViz *cur = &a;
        check(advanceTransition(s, cur, &a, 0.016f), "settled when already showing target");
        check(!s.inTransition && s.timer == 0.0f && cur == &a, "settled state stays clean");
    }
    {
        TransitionState s;
        DummyViz *cur = &a;
        check(!advanceTransition(s, cur, &b, 0.4f), "mid-transition reports active");
        check(s.inTransition && cur == &a, "current held during transition");
        check(!advanceTransition(s, cur, &b, 0.4f), "still active under one second");
        check(advanceTransition(s, cur, &b, 0.4f), "completes past one second");
        check(!s.inTransition && s.timer == 0.0f && cur == &b, "handover resets state");
        check(advanceTransition(s, cur, &b, 0.016f), "settled after handover");
    }
    {
        TransitionState s;
        DummyViz *cur = &a;
        advanceTransition(s, cur, &b, 0.5f);
        check(advanceTransition(s, cur, &a, 0.016f), "retarget to current settles at once");
        check(!s.inTransition && s.timer == 0.0f && cur == &a, "no frozen half-fade");
        check(advanceTransition(s, cur, &a, 0.016f), "stays settled next frame");
    }
    {
        TransitionState s;
        DummyViz *cur = &a;
        advanceTransition(s, cur, &b, 0.9f);
        check(!advanceTransition(s, cur, &c, 0.016f), "retarget elsewhere keeps fading");
        check(s.inTransition && cur == &a, "current held until new target lands");
        check(advanceTransition(s, cur, &c, 0.2f), "new target completes");
        check(cur == &c, "handover follows latest target");
    }
    if (failures == 0) {
        std::printf("ALL PASS\n");
        return 0;
    }
    std::printf("%d FAILURES\n", failures);
    return 1;
}
