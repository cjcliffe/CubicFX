//
//  Visualizer.cpp
//
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#include "Visualizer.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>

// Render size, set from the dialog's render resolution on session start.
int VIZ_WIDTH = 1280;
int VIZ_HEIGHT = 768;

// Load the shader pair, cache every known uniform location, and allocate the
// waveform texture when the fragment shader declares samplerTex.
Visualizer::Visualizer(std::string vsFn, std::string fsFn) {
    timerKick = 0.0f;
    timerMid = 0.0f;
    timerHigh = 0.0f;
    blendAlpha = 1.0f;
    lastBeat = 0;
    randColor[0] = randColor[1] = randColor[2] = 1.0f;
    randColor2[0] = randColor2[1] = randColor2[2] = 1.0f;
    hasOverlay = false;
    hasTex0 = false;
    hasTex1 = false;
    hasTex2 = false;
    hasSampler = false;

    shader = LoadShader(vsFn.c_str(), fsFn.c_str());

    loc_resolution = GetShaderLocation(shader, "resolution");
    loc_time = GetShaderLocation(shader, "time");
    loc_mouse = GetShaderLocation(shader, "mouse");
    loc_vuData = GetShaderLocation(shader, "vuData");
    loc_baseColor = GetShaderLocation(shader, "baseColor");
    loc_vuLow = GetShaderLocation(shader, "vuLow");
    loc_vuMid = GetShaderLocation(shader, "vuMid");
    loc_vuHigh = GetShaderLocation(shader, "vuHigh");
    loc_randColor = GetShaderLocation(shader, "randColor");
    loc_randColor2 = GetShaderLocation(shader, "randColor2");
    loc_beatCounter = GetShaderLocation(shader, "beatCounter");
    loc_beatCounterHalf = GetShaderLocation(shader, "beatCounterHalf");
    loc_beatCounterQuarter = GetShaderLocation(shader, "beatCounterQuarter");
    loc_timerKick = GetShaderLocation(shader, "timerKick");
    loc_timerMid = GetShaderLocation(shader, "timerMid");
    loc_timerHigh = GetShaderLocation(shader, "timerHigh");
    loc_sampleRange = GetShaderLocation(shader, "sampleRange");
    loc_blendAlpha = GetShaderLocation(shader, "blendAlpha");
    loc_samplerTex = GetShaderLocation(shader, "samplerTex");
    loc_overlayImage = GetShaderLocation(shader, "overlayImage");
    loc_tex0 = GetShaderLocation(shader, "tex0");
    loc_tex1 = GetShaderLocation(shader, "tex1");
    loc_tex2 = GetShaderLocation(shader, "tex2");

    if (loc_samplerTex >= 0) {
        Image img = GenImageColor(1024, 1, BLANK);
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R32);
        samplerTexture = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(samplerTexture, TEXTURE_FILTER_BILINEAR);
        hasSampler = true;
    }
}

void Visualizer::setOverlayTexture(Texture2D oTex) {
    overlayTexture = oTex;
    hasOverlay = true;
}

void Visualizer::setTex0Texture(Texture2D tex) {
    tex0 = tex;
    hasTex0 = true;
}

void Visualizer::setTex1Texture(Texture2D tex) {
    tex1 = tex;
    hasTex1 = true;
}

void Visualizer::setTex2Texture(Texture2D tex) {
    tex2 = tex;
    hasTex2 = true;
}

float Visualizer::floatArrayAverage(float *data, int start, int end) {
    float accum = 0;
    for (int i = start; i < end; i++) {
        accum += data[i];
    }
    accum /= (float)end-start;

    return accum;
}

float Visualizer::floatArrayAbsAverage(float *data, int start, int end) {
    float accum = 0;
    for (int i = start; i < end; i++) {
        accum += fabs(data[i]);
    }
    accum /= (float)end-start;

    return accum;
}

void Visualizer::setBlendAlpha(float val) {
    blendAlpha = val;
}

// Refresh shader state for one frame: clock/resolution/mouse, VU band
// levels and timers from vu_data, fresh random colors on each beat, the
// waveform texture upload, and the stored blend alpha. Uniforms the shader
// doesn't declare were cached as -1 and are skipped.
void Visualizer::updateVariables(float time_value, float dt, std::vector<float> &sample_data, std::vector<float> &vu_data, BeatDetektorContest *contest) {
    float last_update = dt;
    if (last_update == 0.0f) {
        last_update = 1.0f/60.0f;
    }

    if (loc_time >= 0) {
        SetShaderValue(shader, loc_time, &time_value, SHADER_UNIFORM_FLOAT);
    }

    if (loc_resolution >= 0) {
        float res[2] = { (float)VIZ_WIDTH, (float)VIZ_HEIGHT };
        SetShaderValue(shader, loc_resolution, res, SHADER_UNIFORM_VEC2);
    }

    if (loc_mouse >= 0) {
        float mouse[2] = { sinf(time_value/10.0f), cosf(time_value/12.0f) };
        SetShaderValue(shader, loc_mouse, mouse, SHADER_UNIFORM_VEC2);
    }

    if (loc_sampleRange >= 0 && !sample_data.empty()) {
        float range = floatArrayAbsAverage(&sample_data[0], 0, (int)sample_data.size());
        SetShaderValue(shader, loc_sampleRange, &range, SHADER_UNIFORM_FLOAT);
    }

    if (loc_vuData >= 0 && vu_data.size() >= 128) {
        SetShaderValueV(shader, loc_vuData, &vu_data[0], SHADER_UNIFORM_FLOAT, 128);
    }

    if (loc_vuLow >= 0 && vu_data.size() >= 128) {
        float v = floatArrayAverage(&vu_data[0], 0, 6);
        SetShaderValue(shader, loc_vuLow, &v, SHADER_UNIFORM_FLOAT);
    }

    if (loc_vuMid >= 0 && vu_data.size() >= 128) {
        float v = floatArrayAverage(&vu_data[0], 50, 60);
        SetShaderValue(shader, loc_vuMid, &v, SHADER_UNIFORM_FLOAT);
    }

    if (loc_vuHigh >= 0 && vu_data.size() >= 128) {
        float v = floatArrayAverage(&vu_data[0], 100, 128);
        SetShaderValue(shader, loc_vuHigh, &v, SHADER_UNIFORM_FLOAT);
    }

    if (lastBeat != contest->beat_counter) {
        randColor[0] = 0.5f + ((float)rand()/(float)RAND_MAX) / 2.0f;
        randColor[1] = 0.5f + ((float)rand()/(float)RAND_MAX) / 2.0f;
        randColor[2] = 0.5f + ((float)rand()/(float)RAND_MAX) / 2.0f;

        randColor2[0] = 0.5f + ((float)rand()/(float)RAND_MAX) / 2.0f;
        randColor2[1] = 0.5f + ((float)rand()/(float)RAND_MAX) / 2.0f;
        randColor2[2] = 0.5f + ((float)rand()/(float)RAND_MAX) / 2.0f;

        lastBeat = contest->beat_counter;
    }

    if (loc_randColor >= 0) {
        SetShaderValue(shader, loc_randColor, randColor, SHADER_UNIFORM_VEC3);
    }

    if (loc_randColor2 >= 0) {
        SetShaderValue(shader, loc_randColor2, randColor2, SHADER_UNIFORM_VEC3);
    }

    if (loc_beatCounter >= 0) {
        SetShaderValue(shader, loc_beatCounter, &contest->beat_counter, SHADER_UNIFORM_INT);
    }

    if (loc_beatCounterHalf >= 0) {
        SetShaderValue(shader, loc_beatCounterHalf, &contest->half_counter, SHADER_UNIFORM_INT);
    }

    if (loc_beatCounterQuarter >= 0) {
        SetShaderValue(shader, loc_beatCounterQuarter, &contest->quarter_counter, SHADER_UNIFORM_INT);
    }

    if (loc_timerKick >= 0 && vu_data.size() >= 128) {
        timerKick += floatArrayAverage(&vu_data[0], 0, 6)*last_update*5.0f;
        SetShaderValue(shader, loc_timerKick, &timerKick, SHADER_UNIFORM_FLOAT);
    }

    if (loc_timerMid >= 0 && vu_data.size() >= 128) {
        timerMid += floatArrayAverage(&vu_data[0], 50, 60)*last_update*5.0f;
        SetShaderValue(shader, loc_timerMid, &timerMid, SHADER_UNIFORM_FLOAT);
    }

    if (loc_timerHigh >= 0 && vu_data.size() >= 128) {
        timerHigh += floatArrayAverage(&vu_data[0], 100, 128)*last_update*5.0f;
        SetShaderValue(shader, loc_timerHigh, &timerHigh, SHADER_UNIFORM_FLOAT);
    }

    if (loc_samplerTex >= 0 && hasSampler && sample_data.size() >= 1024) {
        UpdateTexture(samplerTexture, &sample_data[0]);
    }

    if (loc_blendAlpha >= 0) {
        SetShaderValue(shader, loc_blendAlpha, &blendAlpha, SHADER_UNIFORM_FLOAT);
    }
}

void Visualizer::display(int width, int height)
{
    BeginShaderMode(shader);
    bindTextures();
    DrawRectangle(0, 0, width, height, WHITE);
    EndShaderMode();
}

void Visualizer::bindTextures()
{
    if (loc_samplerTex >= 0 && hasSampler) {
        SetShaderValueTexture(shader, loc_samplerTex, samplerTexture);
    }
    if (loc_overlayImage >= 0 && hasOverlay) {
        SetShaderValueTexture(shader, loc_overlayImage, overlayTexture);
    }
    if (loc_tex0 >= 0 && hasTex0) {
        SetShaderValueTexture(shader, loc_tex0, tex0);
    }
    if (loc_tex1 >= 0 && hasTex1) {
        SetShaderValueTexture(shader, loc_tex1, tex1);
    }
    if (loc_tex2 >= 0 && hasTex2) {
        SetShaderValueTexture(shader, loc_tex2, tex2);
    }
}

void Visualizer::unload()
{
    if (shader.id > 0) {
        UnloadShader(shader);
        shader.id = 0;
    }
    if (hasSampler) {
        UnloadTexture(samplerTexture);
        hasSampler = false;
    }
}
