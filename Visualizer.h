//
//  Visualizer.h
//
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#pragma once

#include <string>
#include <vector>

#include <raylib.h>

#include "BeatDetektor.h"

extern int VIZ_WIDTH;
extern int VIZ_HEIGHT;

class Visualizer {
public:
    Shader shader;

    int loc_resolution;
    int loc_time;
    int loc_mouse;
    int loc_vuData;
    int loc_baseColor;
    int loc_vuLow;
    int loc_vuMid;
    int loc_vuHigh;
    int loc_randColor;
    int loc_randColor2;
    int loc_beatCounter;
    int loc_beatCounterHalf;
    int loc_beatCounterQuarter;
    int loc_timerKick;
    int loc_timerMid;
    int loc_timerHigh;
    int loc_sampleRange;
    int loc_blendAlpha;
    int loc_samplerTex;
    int loc_overlayImage;
    int loc_tex0;
    int loc_tex1;
    int loc_tex2;

    float randColor[3];
    float randColor2[3];
    int lastBeat;

    float timerKick;
    float timerMid;
    float timerHigh;
    float blendAlpha;

    Texture2D overlayTexture;
    bool hasOverlay;
    Texture2D tex0;
    bool hasTex0;
    Texture2D tex1;
    bool hasTex1;
    Texture2D tex2;
    bool hasTex2;

    Texture2D samplerTexture;
    bool hasSampler;

    Visualizer(std::string vsFn, std::string fsFn);

    void setOverlayTexture(Texture2D oTex);
    void setTex0Texture(Texture2D tex);
    void setTex1Texture(Texture2D tex);
    void setTex2Texture(Texture2D tex);

    float floatArrayAverage(float *data, int start, int end);
    float floatArrayAbsAverage(float *data, int start, int end);
    float floatArrayMax(float *data, int start, int end);

    void updateVariables(float time_value, float dt, std::vector<float> &sample_data, std::vector<float> &vu_data, BeatDetektorContest *contest);
    void setBlendAlpha(float val);

    void display(int width, int height);

    void bindTextures();

    void unload();
};
