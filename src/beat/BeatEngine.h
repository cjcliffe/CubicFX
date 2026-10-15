#pragma once

#include "BeatDetektor.h"

extern BeatDetektor *det_low;
extern BeatDetektor *det_high;
extern BeatDetektor *det;
extern BeatDetektorVU *vu;
extern BeatDetektorContest *contest;

void initBD();
void processBD(float timer_seconds, float dt);
