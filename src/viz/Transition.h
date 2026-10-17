// Copyright (c) 2013 Charles J. Cliffe. All rights reserved.

#pragma once

struct TransitionState {
    bool inTransition = false;
    float timer = 0.0f;
};

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
