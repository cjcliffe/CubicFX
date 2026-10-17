// Copyright (c) 2013 Charles J. Cliffe. All rights reserved.

#pragma once

// Crossfade/wipe/zoom state for the session frame loop: whether a blend is
// running and how far into its one-second run it is.
struct TransitionState {
    bool inTransition = false;
    float timer = 0.0f;
};

// Step the transition one frame. Returns true once settled: either the blend
// ran past one second (handing current over to target) or target is already
// showing, which cancels immediately instead of freezing mid-fade.
template <typename T>
bool advanceTransition(TransitionState &state, T *&current, T *target, float dt) {
    if (current == target) {
        state.inTransition = false;
        state.timer = 0.0f;
        return true;
    }
    state.inTransition = true;
    state.timer += dt;
    if (state.timer > 1.0f) {
        state.inTransition = false;
        state.timer = 0.0f;
        current = target;
        return true;
    }
    return false;
}
