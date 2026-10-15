//
//  CubicFX.cpp
//  CubicFX (raylib)
//
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <mutex>

#define NOMINMAX
#define NOGDI
#define NOUSER

#include <raylib.h>
#include <RtAudio.h>

#include "Visualizer.h"
#include "FFT.h"
#include "BeatDetektor.h"

#define SRATE 44100
#define BUF 2048

using namespace std;

static RtAudio *audioIn = nullptr;
static unsigned int audioChannels = 0;
static vector<float> staging;
static vector<float> readyBlock;
static bool blockFresh = false;
static bool audioRunning = false;
static mutex audioMutex;

vector<float> sample_data;
vector<float> fft_data;
vector<float> fft_collapse;

BeatDetektor *det_low;
BeatDetektor *det_high;
BeatDetektor *det;
BeatDetektorVU *vu;
BeatDetektorContest *contest;

float alphaVal = 1.0;

struct AudioDevInfo {
    unsigned int id;
    string name;
    unsigned int inChannels;
};

static vector<AudioDevInfo> listInputDevices() {
    vector<AudioDevInfo> devs;
    RtAudio probe;
    for (unsigned int id : probe.getDeviceIds()) {
        RtAudio::DeviceInfo info = probe.getDeviceInfo(id);
        if (info.inputChannels > 0) {
            devs.push_back({ id, info.name, info.inputChannels });
        }
    }
    return devs;
}

static int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames,
    double streamTime, RtAudioStreamStatus status, void *userData) {
    if (status) {
        printf("Audio stream overflow.\n");
    }
    if (inputBuffer == nullptr) {
        return 0;
    }
    float *input = (float *)inputBuffer;
    lock_guard<mutex> lock(audioMutex);
    for (unsigned int i = 0; i < nFrames; i++) {
        float mono = 0.0f;
        for (unsigned int c = 0; c < audioChannels; c++) {
            mono += input[i * audioChannels + c];
        }
        mono /= (float)audioChannels;
        staging.push_back(mono);
    }
    while (staging.size() >= BUF) {
        readyBlock.assign(staging.begin(), staging.begin() + BUF);
        staging.erase(staging.begin(), staging.begin() + BUF);
        blockFresh = true;
        if (staging.size() > BUF) {
            staging.erase(staging.begin(), staging.end() - BUF);
        }
    }
    return 0;
}

static bool openAudio(unsigned int deviceId) {
    audioIn = new RtAudio();
    RtAudio::DeviceInfo info = audioIn->getDeviceInfo(deviceId);
    if (info.inputChannels == 0) {
        printf("Selected device has no input channels.\n");
        delete audioIn;
        audioIn = nullptr;
        return false;
    }
    audioChannels = info.inputChannels >= 2 ? 2 : 1;

    unsigned int sampleRate = SRATE;
    bool rateOk = false;
    for (unsigned int r : info.sampleRates) {
        if (r == SRATE) {
            rateOk = true;
            break;
        }
    }
    if (!rateOk && !info.sampleRates.empty()) {
        sampleRate = info.sampleRates[0];
        printf("Device does not support %d Hz, using %u Hz.\n", SRATE, sampleRate);
    }

    RtAudio::StreamParameters params;
    params.deviceId = deviceId;
    params.nChannels = audioChannels;
    params.firstChannel = 0;
    unsigned int bufferFrames = BUF;

    RtAudioErrorType err = audioIn->openStream(nullptr, &params, RTAUDIO_FLOAT32,
        sampleRate, &bufferFrames, &audioCallback);
    if (err != RTAUDIO_NO_ERROR) {
        printf("Could not open audio stream: %s\n", audioIn->getErrorText().c_str());
        delete audioIn;
        audioIn = nullptr;
        return false;
    }
    err = audioIn->startStream();
    if (err != RTAUDIO_NO_ERROR) {
        printf("Could not start audio stream: %s\n", audioIn->getErrorText().c_str());
        audioIn->closeStream();
        delete audioIn;
        audioIn = nullptr;
        return false;
    }

    sample_data.assign(BUF, 0.0f);
    fft_data.assign(BUF, 0.0f);
    readyBlock.assign(BUF, 0.0f);
    audioRunning = true;
    return true;
}

static void closeAudio() {
    if (audioIn != nullptr) {
        if (audioIn->isStreamRunning()) {
            audioIn->stopStream();
        }
        if (audioIn->isStreamOpen()) {
            audioIn->closeStream();
        }
        delete audioIn;
        audioIn = nullptr;
    }
    audioRunning = false;
}

static float normGain = 1.0f;
static bool agcEnabled = true;

static void normalizeAudioBlock(vector<float> &block) {
    double mean = 0.0;
    for (float v : block) {
        mean += v;
    }
    mean /= (double)block.size();

    float peak = 0.0f;
    for (float v : block) {
        float a = fabsf((float)(v - mean));
        if (a > peak) {
            peak = a;
        }
    }

    if (peak > 1e-4f) {
        float target = 0.9f / peak;
        if (target > 32.0f) {
            target = 32.0f;
        }
        if (target < 0.5f) {
            target = 0.5f;
        }
        float rate = (target > normGain) ? 0.4f : 0.03f;
        normGain += (target - normGain) * rate;
    }

    for (float &v : block) {
        v = tanhf(normGain * (float)(v - mean));
    }
}

static void captureAudio() {
    if (!audioRunning) {
        return;
    }
    bool fresh = false;
    {
        lock_guard<mutex> lock(audioMutex);
        if (blockFresh) {
            sample_data = readyBlock;
            blockFresh = false;
            fresh = true;
        }
    }
    if (!fresh) {
        return;
    }

    if (agcEnabled) {
        normalizeAudioBlock(sample_data);
    }

    fft_data = sample_data;

    DanielsonLanczos <(BUF/2), float> dlfft;

    unsigned long n, m, j, i;

    n = (BUF/2)<<1;
    j=1;
    for (i=1; i<n; i+=2) {
        if (j>i) {
            swap(fft_data[j-1], fft_data[i-1]);
            swap(fft_data[j], fft_data[i]);
        }
        m = (BUF/2);
        while (m>=2 && j>m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    };

    dlfft.apply(&fft_data[0]);
}

void initBD() {
    det_low = new BeatDetektor(90,160);
    det_high = new BeatDetektor(140, 260);

    det = det_low;

    vu = new BeatDetektorVU();

    contest = new BeatDetektorContest();
    contest->no_contest_decay = false;
    contest->finish_line = 1000;
}

void processBD(float timer_seconds, float dt) {
    int x;

    fft_collapse.clear();
    for (x = 0; x < 256; x++)
    {
        fft_collapse.push_back(fft_data[x]);
    }
    for (x = BUF-256; x < BUF; x++)
    {
        fft_collapse.push_back(fft_data[x]);
    }

    det_low->process(timer_seconds,fft_collapse);
    det_high->process(timer_seconds, fft_collapse);
    contest->process(timer_seconds, det);

    if (contest->win_bpm_int)
        vu->process(det,dt,((float)contest->win_bpm_int/10.0));

    contest->run();
}

Visualizer *currentViz = NULL;
vector<Visualizer *> visualizers;
vector<Texture2D> overlays;

static void loadOverlays() {
    overlays.push_back(LoadTexture("png/OFFSET.png"));
    overlays.push_back(LoadTexture("png/JACK.png"));
    overlays.push_back(LoadTexture("png/synthamesk.png"));
    overlays.push_back(LoadTexture("png/C64.png"));
    overlays.push_back(LoadTexture("png/LAFO.png"));
    overlays.push_back(LoadTexture("png/LAFOLOGO.png"));
}

Visualizer *overlayImage;
Visualizer *targetViz = NULL;

float transition_timer = 0.0;
int transition = 0;
bool in_transition = false;

enum transitionType {
    TRANS_CROSSFADE,
    TRANS_WIPERIGHT,
    TRANS_WIPELEFT,
    TRANS_WIPEDOWN,
    TRANS_WIPEUP,
    TRANS_ZOOMIN,
    TRANS_MAX
};
transitionType trans_type;

int lastImage = -1;
int thisImage = 0;
float overlayAlpha = 0.0f;
float overlayDelta = 0.0f;
bool overlayEnabled = false;
float autoTimer = 0;
float autoTimerLimit = 30;
bool manualOverride = false;

void vizRandom() {
    int nextViz = (int)floor(((float)rand() / (float)RAND_MAX)*visualizers.size());
    if (nextViz > (int)visualizers.size() - 1) nextViz = (int)visualizers.size() - 1;
    targetViz = visualizers[nextViz];
    if (currentViz == NULL) {
        currentViz = targetViz;
    }
}

void transRandom() {
    trans_type = (transitionType)(int)floor(((float)rand() / (float)RAND_MAX)*TRANS_MAX);
}

static bool shiftHeld() {
    return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

static Visualizer *vizKey(int key) {
    switch (key) {
    case KEY_ONE: return visualizers[0];
    case KEY_TWO: return visualizers[1];
    case KEY_THREE: return visualizers[2];
    case KEY_FOUR: return visualizers[3];
    case KEY_FIVE: return visualizers[4];
    case KEY_SIX: return visualizers[5];
    case KEY_SEVEN: return visualizers[6];
    case KEY_EIGHT: return visualizers[7];
    case KEY_NINE: return visualizers[8];
    case KEY_ZERO: return visualizers[9];
    case KEY_Q: return visualizers[10];
    case KEY_W: return visualizers[11];
    case KEY_E: return visualizers[12];
    case KEY_R: return visualizers[13];
    case KEY_T: return visualizers[14];
    case KEY_Y: return visualizers[15];
    case KEY_U: return visualizers[16];
    case KEY_I: return visualizers[17];
    case KEY_O: return visualizers[18];
    case KEY_P: return visualizers[19];
    case KEY_A: return visualizers[20];
    case KEY_S: return visualizers[21];
    case KEY_D: return visualizers[22];
    case KEY_F: return visualizers[23];
    case KEY_G: return visualizers[24];
    case KEY_H: return visualizers[25];
    case KEY_J: return visualizers[26];
    case KEY_K: return visualizers[27];
    case KEY_L: return visualizers[28];
    case KEY_Z: return visualizers[29];
    case KEY_X: return visualizers[30];
    case KEY_C: return visualizers[31];
    case KEY_V: return visualizers[32];
    case KEY_B: return visualizers[33];
    case KEY_N: return visualizers[34];
    case KEY_M: return visualizers[35];
    case KEY_LEFT_BRACKET: return visualizers[36];
    case KEY_RIGHT_BRACKET: return visualizers[37];
    case KEY_SEMICOLON: return visualizers[38];
    case KEY_APOSTROPHE: return visualizers[39];
    case KEY_MINUS: return visualizers[40];
    case KEY_EQUAL: return visualizers[41];
    default: return NULL;
    }
}

static const int vizKeys[] = {
    KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE,
    KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE, KEY_ZERO,
    KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U,
    KEY_I, KEY_O, KEY_P, KEY_A, KEY_S, KEY_D, KEY_F,
    KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_Z, KEY_X,
    KEY_C, KEY_V, KEY_B, KEY_N, KEY_M,
    KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET,
    KEY_SEMICOLON, KEY_APOSTROPHE, KEY_MINUS, KEY_EQUAL
};

static void pollKeys() {
    if (shiftHeld()) {
        bool doChange = false;
        thisImage = -1;
        if (IsKeyPressed(KEY_ONE)) { overlayImage->setOverlayTexture(overlays[0]); thisImage = 0; doChange = true; }
        if (IsKeyPressed(KEY_TWO)) { overlayImage->setOverlayTexture(overlays[1]); thisImage = 1; doChange = true; }
        if (IsKeyPressed(KEY_THREE)) { overlayImage->setOverlayTexture(overlays[2]); thisImage = 2; doChange = true; }
        if (IsKeyPressed(KEY_FOUR)) { overlayImage->setOverlayTexture(overlays[3]); thisImage = 3; doChange = true; }
        if (IsKeyPressed(KEY_FIVE)) { overlayImage->setOverlayTexture(overlays[4]); thisImage = 4; doChange = true; }
        if (IsKeyPressed(KEY_SIX)) { overlayImage->setOverlayTexture(overlays[5]); thisImage = 5; doChange = true; }
        if (IsKeyPressed(KEY_ZERO)) {
            overlayEnabled = !overlayEnabled;
            overlayDelta = 0;
        }

        if (doChange && thisImage == lastImage) {
            overlayDelta = overlayDelta ? -overlayDelta : -1.0f;
        }

        if (doChange && !overlayEnabled) {
            overlayEnabled = true;
            overlayDelta = -1.0f;
        }

        if (thisImage != -1) lastImage = thisImage;
    } else {
        Visualizer *tempViz = targetViz;
        for (int k : vizKeys) {
            if (IsKeyPressed(k)) {
                targetViz = vizKey(k);
                break;
            }
        }
        if (IsKeyPressed(KEY_SPACE)) {
            vizRandom();
            transRandom();
            manualOverride = false;
            autoTimer = 0;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            printf("Low mode\n");
            det = det_low;
        }
        if (IsKeyPressed(KEY_UP)) {
            printf("High mode\n");
            det = det_high;
        }

        if (targetViz != tempViz) {
            manualOverride = true;
            transRandom();
        }
    }
}

struct SetupChoice {
    int monitor = 0;
    bool fullscreen = false;
    int width = 1280;
    int height = 768;
    int resPreset = 4;
    unsigned int audioDevice = 0;
    int audioIndex = 0;
    bool agc = true;
};

static const int resPresets[][2] = {
    { 1920, 1080 },
    { 1600, 900 },
    { 1366, 768 },
    { 1280, 720 },
    { 1280, 768 },
    { 1024, 768 },
    { 800, 600 },
};
static const int numResPresets = 7;

static string shortName(const string &name, size_t maxLen) {
    if (name.size() <= maxLen) {
        return name;
    }
    return name.substr(0, maxLen - 3) + "...";
}

static bool uiButton(Rectangle r, const char *label, bool selected) {
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, r);
    bool clicked = hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    Color fill = selected ? SKYBLUE : (hover ? GRAY : DARKGRAY);
    DrawRectangleRec(r, fill);
    DrawRectangleLinesEx(r, 1, LIGHTGRAY);
    Color textColor = selected ? BLACK : RAYWHITE;
    DrawText(label, (int)r.x + 8, (int)(r.y + r.height / 2 - 10), 20, textColor);
    return clicked;
}

static const int DIALOG_W = 660;
static const int DIALOG_H = 720;

static bool runSetupDialog(SetupChoice &choice, vector<AudioDevInfo> &audioDevs) {
    const int dlgW = DIALOG_W;
    const int dlgH = DIALOG_H;

    int monitorCount = GetMonitorCount();
    if (choice.monitor < 0 || choice.monitor >= monitorCount) {
        choice.monitor = 0;
    }
    if (!audioDevs.empty() && (choice.audioIndex < 0 || choice.audioIndex >= (int)audioDevs.size())) {
        choice.audioIndex = 0;
        choice.audioDevice = audioDevs[0].id;
    }

    bool done = false;
    bool cancelled = false;
    float audioScroll = 0.0f;
    bool draggingScroll = false;
    while (!WindowShouldClose() && !done && !cancelled) {
        if (IsKeyPressed(KEY_ENTER)) {
            done = true;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            cancelled = true;
        }

        float y = 12;
        BeginDrawing();
        ClearBackground(BLACK);

        DrawText("CubicFX Setup", 16, (int)y, 28, RAYWHITE);
        y += 40;

        DrawText("Display monitor:", 16, (int)y, 20, LIGHTGRAY);
        y += 26;
        for (int m = 0; m < monitorCount && m < 4; m++) {
            char label[160];
            snprintf(label, sizeof(label), "%d: %s (%dx%d)", m + 1,
                shortName(GetMonitorName(m), 34).c_str(),
                GetMonitorWidth(m), GetMonitorHeight(m));
            if (uiButton({ 16, y, dlgW - 32, 30 }, label, m == choice.monitor)) {
                choice.monitor = m;
            }
            y += 36;
        }

        DrawText("Mode:", 16, (int)y, 20, LIGHTGRAY);
        y += 26;
        if (uiButton({ 16, y, (dlgW - 40) / 2, 30 }, "Windowed", !choice.fullscreen)) {
            choice.fullscreen = false;
        }
        if (uiButton({ 16 + (dlgW - 40) / 2 + 8, y, (dlgW - 40) / 2, 30 }, "Fullscreen", choice.fullscreen)) {
            choice.fullscreen = true;
        }
        y += 36;

        if (choice.fullscreen) {
            char label[128];
            snprintf(label, sizeof(label), "Fullscreen uses monitor native: %dx%d",
                GetMonitorWidth(choice.monitor), GetMonitorHeight(choice.monitor));
            DrawText(label, 16, (int)y, 20, GRAY);
            y += 28;
        } else {
            DrawText("Resolution:", 16, (int)y, 20, LIGHTGRAY);
            y += 26;
            for (int i = 0; i < numResPresets; i++) {
                char label[32];
                snprintf(label, sizeof(label), "%dx%d", resPresets[i][0], resPresets[i][1]);
                float bw = (dlgW - 32 - 3 * 8) / 4;
                float bx = 16 + (i % 4) * (bw + 8);
                float by = y + (i / 4) * 36;
                if (uiButton({ bx, by, bw, 30 }, label, i == choice.resPreset)) {
                    choice.resPreset = i;
                    choice.width = resPresets[i][0];
                    choice.height = resPresets[i][1];
                }
            }
            y += ((numResPresets + 3) / 4) * 36 + 6;
        }

        DrawText("Audio input:", 16, (int)y, 20, LIGHTGRAY);
        Rectangle agcHit = { (float)dlgW - 116 - 8 - 160, y - 4, 160, 26 };
        Vector2 agcMouse = GetMousePosition();
        bool agcHover = CheckCollisionPointRec(agcMouse, agcHit);
        if (agcHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            choice.agc = !choice.agc;
        }
        float agcBoxY = agcHit.y + 5;
        DrawRectangleLinesEx({ agcHit.x, agcBoxY, 16, 16 }, 1, LIGHTGRAY);
        if (choice.agc) {
            DrawRectangle((int)agcHit.x + 3, (int)agcBoxY + 3, 10, 10, SKYBLUE);
        }
        DrawText("Auto Gain", (int)agcHit.x + 22, (int)(agcHit.y + 3), 20, agcHover ? RAYWHITE : LIGHTGRAY);
        if (uiButton({ (float)dlgW - 116, y - 4, 100, 26 }, "Refresh", false)) {
            unsigned int keepId = choice.audioDevice;
            audioDevs = listInputDevices();
            choice.audioIndex = -1;
            for (int i = 0; i < (int)audioDevs.size(); i++) {
                if (audioDevs[i].id == keepId) {
                    choice.audioIndex = i;
                    break;
                }
            }
            if (choice.audioIndex < 0 && !audioDevs.empty()) {
                choice.audioIndex = 0;
            }
            choice.audioDevice = audioDevs.empty() ? 0 : audioDevs[choice.audioIndex].id;
        }
        y += 26;
        if (audioDevs.empty()) {
            DrawText("No input devices found.", 16, (int)y, 20, RED);
            y += 30;
        } else {
            const float rowH = 36.0f;
            const float barW = 12.0f;
            float panelH = ((float)dlgH - 96.0f) - y;
            if (panelH < 2 * rowH - 6) {
                panelH = 2 * rowH - 6;
            }
            Rectangle panel = { 16, y, (float)dlgW - 32, panelH };
            float contentH = (float)audioDevs.size() * rowH;
            bool showBar = contentH > panel.height;
            float maxScroll = contentH - panel.height;
            if (maxScroll < 0) {
                maxScroll = 0;
            }

            Vector2 wheelMouse = GetMousePosition();
            bool overPanel = CheckCollisionPointRec(wheelMouse, panel);
            if (overPanel && !draggingScroll) {
                audioScroll -= GetMouseWheelMove() * rowH;
            }
            if (audioScroll < 0) {
                audioScroll = 0;
            }
            if (audioScroll > maxScroll) {
                audioScroll = maxScroll;
            }

            float thumbH = showBar ? panel.height * (panel.height / contentH) : 0.0f;
            if (thumbH < 20.0f) {
                thumbH = 20.0f;
            }
            Rectangle track = { panel.x + panel.width - barW, panel.y, barW, panel.height };

            if (showBar && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(wheelMouse, track)) {
                draggingScroll = true;
            }
            if (draggingScroll) {
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && (panel.height - thumbH) >= 1.0f) {
                    float frac = (wheelMouse.y - panel.y - thumbH * 0.5f) / (panel.height - thumbH);
                    if (frac < 0) {
                        frac = 0;
                    }
                    if (frac > 1) {
                        frac = 1;
                    }
                    audioScroll = frac * maxScroll;
                } else if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    draggingScroll = false;
                }
            }

            float thumbY = panel.y;
            if (showBar && maxScroll > 0) {
                thumbY = panel.y + (panel.height - thumbH) * (audioScroll / maxScroll);
            }

            DrawRectangleRec(panel, Color{ 16, 16, 16, 255 });
            BeginScissorMode((int)panel.x, (int)panel.y, (int)panel.width, (int)panel.height);
            for (int i = 0; i < (int)audioDevs.size(); i++) {
                char label[160];
                snprintf(label, sizeof(label), "%s (%uch)", shortName(audioDevs[i].name, 40).c_str(), audioDevs[i].inChannels);
                float rowW = panel.width - (showBar ? barW + 4 : 0);
                Rectangle row = { panel.x, panel.y + i * rowH - audioScroll, rowW, 30 };
                if (uiButton(row, label, i == choice.audioIndex) && overPanel && !draggingScroll) {
                    choice.audioIndex = i;
                    choice.audioDevice = audioDevs[i].id;
                }
            }
            EndScissorMode();

            if (showBar) {
                DrawRectangleRec(track, Color{ 48, 48, 48, 255 });
                DrawRectangleRec({ track.x, thumbY, barW, thumbH }, LIGHTGRAY);
            }

            y += panel.height + 10;
        }

        y += 10;
        if (uiButton({ 16, dlgH - 52, dlgW - 32, 36 }, "Start", false)) {
            done = true;
        }
        DrawText("Enter = Start    Esc = Quit", 16, dlgH - 78, 20, GRAY);

        EndDrawing();
    }

    return done;
}

static void drawVizInto(RenderTexture2D rt, Visualizer *viz, float t, float dt) {
    viz->updateVariables(t, dt, sample_data, vu->vu_levels, contest);
    BeginTextureMode(rt);
    ClearBackground(BLANK);
    viz->display(VIZ_WIDTH, VIZ_HEIGHT);
    EndTextureMode();
}

static bool runVisualizer(SetupChoice &choice)
{
    currentViz = NULL;
    targetViz = NULL;
    transition_timer = 0.0f;
    in_transition = false;
    lastImage = -1;
    thisImage = 0;
    overlayAlpha = 0.0f;
    overlayDelta = 0.0f;
    overlayEnabled = false;
    autoTimer = 0;
    manualOverride = false;
    alphaVal = 1.0f;
    sample_data.assign(BUF, 0.0f);
    fft_data.assign(BUF, 0.0f);
    normGain = 1.0f;
    agcEnabled = choice.agc;
    {
        lock_guard<mutex> lock(audioMutex);
        staging.clear();
        blockFresh = false;
    }

    if (choice.fullscreen) {
        int monW = GetMonitorWidth(choice.monitor);
        int monH = GetMonitorHeight(choice.monitor);
        SetWindowSize(monW, monH);
        SetWindowMonitor(choice.monitor);
        if (!IsWindowFullscreen()) {
            ToggleFullscreen();
        }
        VIZ_WIDTH = GetScreenWidth();
        VIZ_HEIGHT = GetScreenHeight();
    } else {
        if (IsWindowFullscreen()) {
            ToggleFullscreen();
        }
        SetWindowSize(choice.width, choice.height);
        SetWindowMonitor(choice.monitor);
        VIZ_WIDTH = choice.width;
        VIZ_HEIGHT = choice.height;
    }
    SetWindowTitle("BeatDetektor CubicFX");

    initBD();

    Visualizer vizRgbFlares("shaders/vertex_common.vs", "shaders/rgb_flares_in_tunnel.fs");
    Visualizer sparklingSine("shaders/vertex_common.vs", "shaders/sparkling_sine_wave.fs");
    Visualizer sparklingBlocks("shaders/vertex_common.vs", "shaders/sparkling_blocks.fs");
    Visualizer flareWave("shaders/vertex_common.vs", "shaders/flare_wave.fs");
    Visualizer rbCrawl("shaders/vertex_common.vs", "shaders/red_blue_crawl_pattern.fs");
    Visualizer rmTunnel("shaders/vertex_common.vs", "shaders/raymarch_tunnel.fs");
    Visualizer discoFloor("shaders/vertex_common.vs", "shaders/discofloor_ceiling.fs");
    Visualizer flareTunnel("shaders/vertex_common.vs", "shaders/flare_tunnel.fs");
    Visualizer mandelBlob("shaders/vertex_common.vs","shaders/raymarch_fractal3d.fs");
    Visualizer lineZoom("shaders/vertex_common.vs","shaders/raymarch_lines_zoom.fs");
    Visualizer hexField("shaders/vertex_common.vs", "shaders/hex_twist_field.fs");
    Visualizer discoBall("shaders/vertex_common.vs", "shaders/discoball.fs");
    Visualizer cubeArray("shaders/vertex_common.vs", "shaders/cube_array.fs");
    Visualizer planeDeformFly("shaders/vertex_common.vs", "shaders/planed_fly.fs");
    Visualizer planeDeformReliefTunnel("shaders/vertex_common.vs", "shaders/planed_relief_tun.fs");
    Visualizer planeDeformSquareTunnel("shaders/vertex_common.vs", "shaders/planed_square_tun.fs");
    Visualizer hexTunnel("shaders/vertex_common.vs", "shaders/hex_tunnel.fs");
    Visualizer cubeScape("shaders/vertex_common.vs", "shaders/rm_cubescape.fs");
    Visualizer inversionMachine("shaders/vertex_common.vs", "shaders/inversion_machine.fs");
    Visualizer mengerJourney("shaders/vertex_common.vs", "shaders/menger_journey.fs");
    Visualizer aLotOfSpheres("shaders/vertex_common.vs", "shaders/a_lot_of_spheres.fs");
    Visualizer fractalGears("shaders/vertex_common.vs", "shaders/fractal_gears.fs");
    Visualizer metaBalls("shaders/vertex_common.vs", "shaders/metaballs.fs");
    Visualizer particleTracing("shaders/vertex_common.vs", "shaders/particle_tracing.fs");
    Visualizer flyingCubes("shaders/vertex_common.vs", "shaders/flying_cubes.fs");
    Visualizer spaceShip("shaders/vertex_common.vs", "shaders/spaceship.fs");
    Visualizer spaceRings("shaders/vertex_common.vs", "shaders/space_rings.fs");
    Visualizer hologram("shaders/vertex_common.vs", "shaders/hologram.fs");
    Visualizer musicBall("shaders/vertex_common.vs", "shaders/music_ball.fs");
    Visualizer ledSpectrum("shaders/vertex_common.vs", "shaders/led_spectrum.fs");
    Visualizer kifs2D("shaders/vertex_common.vs", "shaders/2dkifs.fs");
    Visualizer planeDTunEffect("shaders/vertex_common.vs", "shaders/planed_tunnel_effect.fs");
    Visualizer noxiousBox("shaders/vertex_common.vs", "shaders/noxious_box.fs");
    Visualizer waveFloor("shaders/vertex_common.vs", "shaders/wave_floor.fs");
    Visualizer hexField2("shaders/vertex_common.vs", "shaders/hex_field.fs");
    Visualizer spaceRace("shaders/vertex_common.vs", "shaders/space_race.fs");

    Texture2D tex_noise = LoadTexture("png/tex_noise1.png");
    Texture2D tex_noise2 = LoadTexture("png/tex_noise2.png");
    Texture2D tex_tex06 = LoadTexture("png/tex_tex06.png");
    Texture2D tex_tex07 = LoadTexture("png/tex_tex07.png");

    Visualizer truchetTentacles("shaders/vertex_common.vs", "shaders/truchet_tentacles.fs");
    Visualizer circlePattern("shaders/vertex_common.vs", "shaders/circle_pattern.fs");
    circlePattern.setTex0Texture(tex_noise);

    Visualizer fireShader("shaders/vertex_common.vs", "shaders/fire.fs");
    Visualizer dubstepMyAss("shaders/vertex_common.vs", "shaders/dubstepmyass.fs");

    Visualizer voxelEdges("shaders/vertex_common.vs", "shaders/voxel_edges.fs");
    voxelEdges.setTex0Texture(tex_noise2);
    voxelEdges.setTex1Texture(tex_tex07);
    voxelEdges.setTex2Texture(tex_tex06);

    Visualizer noiseBlur("shaders/vertex_common.vs", "shaders/noise_blur.fs");

    overlayImage = new Visualizer("shaders/vertex_common.vs", "shaders/overlay_image.fs");

    loadOverlays();

    if (!openAudio(choice.audioDevice)) {
        printf("Continuing without audio input.\n");
    }

    visualizers.push_back(&vizRgbFlares);
    visualizers.push_back(&sparklingSine);
    visualizers.push_back(&sparklingBlocks);
    visualizers.push_back(&flareWave);
    visualizers.push_back(&rbCrawl);
    visualizers.push_back(&rmTunnel);
    visualizers.push_back(&discoFloor);
    visualizers.push_back(&flareTunnel);
    visualizers.push_back(&mandelBlob);
    visualizers.push_back(&lineZoom);
    visualizers.push_back(&hexField);
    visualizers.push_back(&discoBall);
    visualizers.push_back(&cubeArray);
    visualizers.push_back(&planeDeformFly);
    visualizers.push_back(&planeDeformReliefTunnel);
    visualizers.push_back(&planeDeformSquareTunnel);
    visualizers.push_back(&hexTunnel);
    visualizers.push_back(&cubeScape);
    visualizers.push_back(&aLotOfSpheres);
    visualizers.push_back(&fractalGears);
    visualizers.push_back(&inversionMachine);
    visualizers.push_back(&mengerJourney);
    visualizers.push_back(&metaBalls);
    visualizers.push_back(&particleTracing);
    visualizers.push_back(&flyingCubes);
    visualizers.push_back(&spaceShip);
    visualizers.push_back(&spaceRings);
    visualizers.push_back(&hologram);
    visualizers.push_back(&musicBall);
    visualizers.push_back(&ledSpectrum);
    visualizers.push_back(&kifs2D);
    visualizers.push_back(&planeDTunEffect);
    visualizers.push_back(&noxiousBox);
    visualizers.push_back(&waveFloor);
    visualizers.push_back(&hexField2);
    visualizers.push_back(&spaceRace);
    visualizers.push_back(&truchetTentacles);
    visualizers.push_back(&circlePattern);
    visualizers.push_back(&fireShader);
    visualizers.push_back(&dubstepMyAss);
    visualizers.push_back(&voxelEdges);
    visualizers.push_back(&noiseBlur);

    vizRandom();
    transRandom();

    RenderTexture2D rtCurrent = LoadRenderTexture(VIZ_WIDTH, VIZ_HEIGHT);
    RenderTexture2D rtTarget = LoadRenderTexture(VIZ_WIDTH, VIZ_HEIGHT);
    Rectangle rtSource = { 0, 0, (float)VIZ_WIDTH, -(float)VIZ_HEIGHT };
    Rectangle rtDest = { 0, 0, (float)VIZ_WIDTH, (float)VIZ_HEIGHT };
    Vector2 rtOrigin = { 0, 0 };

    SetTargetFPS(60);

    bool toMenu = false;
    while (!WindowShouldClose() && !toMenu)
    {
        float dt = GetFrameTime();
        float t = (float)GetTime();

        captureAudio();
        processBD(t, dt);

        pollKeys();

        if (IsKeyPressed(KEY_ESCAPE)) {
            toMenu = true;
        }

        float alphaDir = 0.0f;
        if (IsKeyDown(KEY_PAGE_DOWN) || IsKeyDown(KEY_COMMA)) {
            alphaDir = shiftHeld() ? 4.0f : 1.0f;
        }
        if (IsKeyDown(KEY_PAGE_UP) || IsKeyDown(KEY_PERIOD)) {
            alphaDir = shiftHeld() ? -4.0f : -1.0f;
        }
        if (alphaDir != 0.0f) {
            alphaVal += alphaDir*dt*0.1f;
            if (alphaVal > 1.0f) {
                alphaVal = 1.0f;
            }
            if (alphaVal < 0.05f) {
                alphaVal = 0.05f;
            }
            printf("%f\n", alphaVal);
        }

        if (currentViz != targetViz) {
            in_transition = true;
            transition_timer += dt;
            if (transition_timer > 1.0f) {
                in_transition = false;
                transition_timer = 0;
                currentViz = targetViz;
            }
        }

        if (in_transition) {
            currentViz->setBlendAlpha((1.0f - transition_timer)*alphaVal);
            drawVizInto(rtCurrent, currentViz, t, dt);

            targetViz->setBlendAlpha(alphaVal*transition_timer);
            drawVizInto(rtTarget, targetViz, t, dt);
        }
        else {
            currentViz->setBlendAlpha(alphaVal);
            drawVizInto(rtCurrent, currentViz, t, dt);
        }

        if (overlayDelta) {
            if (overlayEnabled) {
                overlayAlpha += overlayDelta * dt;
            }

            if (overlayAlpha < -1.0f) {
                overlayAlpha = -1.0f;
            }
            else if (overlayAlpha > 1.0f) {
                overlayAlpha = 1.0f;
            }
        }
        else if (overlayAlpha) {
            overlayAlpha -= overlayAlpha * dt;
            if (fabs(overlayAlpha) < 0.01f) {
                overlayAlpha = 0.0f;
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (in_transition) {
            float k = transition_timer;
            switch (trans_type) {
            case TRANS_CROSSFADE:
                DrawTexturePro(rtCurrent.texture, rtSource, rtDest, rtOrigin, 0.0f, WHITE);
                DrawTexturePro(rtTarget.texture, rtSource, rtDest, rtOrigin, 0.0f, ColorAlpha(WHITE, k));
                break;
            case TRANS_WIPERIGHT:
                DrawTexturePro(rtCurrent.texture, rtSource, rtDest, rtOrigin, 0.0f, WHITE);
                BeginScissorMode(0, 0, (int)(k*VIZ_WIDTH), VIZ_HEIGHT);
                DrawTexturePro(rtTarget.texture, rtSource, rtDest, rtOrigin, 0.0f, WHITE);
                EndScissorMode();
                break;
            case TRANS_WIPELEFT:
                DrawTexturePro(rtCurrent.texture, rtSource, rtDest, rtOrigin, 0.0f, WHITE);
                BeginScissorMode(VIZ_WIDTH - (int)(k*VIZ_WIDTH), 0, (int)(k*VIZ_WIDTH), VIZ_HEIGHT);
                DrawTexturePro(rtTarget.texture, rtSource, rtDest, rtOrigin, 0.0f, WHITE);
                EndScissorMode();
                break;
            case TRANS_WIPEDOWN:
                DrawTexturePro(rtCurrent.texture, rtSource, rtDest, rtOrigin, 0.0f, WHITE);
                BeginScissorMode(0, 0, VIZ_WIDTH, (int)(k*VIZ_HEIGHT));
                DrawTexturePro(rtTarget.texture, rtSource, rtDest, rtOrigin, 0.0f, WHITE);
                EndScissorMode();
                break;
            case TRANS_WIPEUP:
                DrawTexturePro(rtCurrent.texture, rtSource, rtDest, rtOrigin, 0.0f, WHITE);
                BeginScissorMode(0, VIZ_HEIGHT - (int)(k*VIZ_HEIGHT), VIZ_WIDTH, (int)(k*VIZ_HEIGHT));
                DrawTexturePro(rtTarget.texture, rtSource, rtDest, rtOrigin, 0.0f, WHITE);
                EndScissorMode();
                break;
            case TRANS_ZOOMIN: {
                DrawTexturePro(rtCurrent.texture, rtSource, rtDest, rtOrigin, 0.0f, WHITE);
                float zw = k*VIZ_WIDTH;
                float zh = k*VIZ_HEIGHT;
                Rectangle zoomDest = { (VIZ_WIDTH - zw) / 2.0f, (VIZ_HEIGHT - zh) / 2.0f, zw, zh };
                DrawTexturePro(rtTarget.texture, rtSource, zoomDest, rtOrigin, 0.0f, WHITE);
                break;
            }
            default:
                DrawTexturePro(rtCurrent.texture, rtSource, rtDest, rtOrigin, 0.0f, WHITE);
                break;
            }
        }
        else {
            DrawTexturePro(rtCurrent.texture, rtSource, rtDest, rtOrigin, 0.0f, WHITE);
        }

        if (overlayEnabled || overlayAlpha != 0.0f) {
            overlayImage->setBlendAlpha(overlayAlpha);
            overlayImage->updateVariables(t, dt, sample_data, vu->vu_levels, contest);
            overlayImage->display(VIZ_WIDTH, VIZ_HEIGHT);
        }

        EndDrawing();

        if (!in_transition) autoTimer += dt;
        if (autoTimer > autoTimerLimit && !manualOverride) {
            autoTimer = 0;
            vizRandom();
        }

        if (!manualOverride) {
            alphaVal = 1.0f + (sinf(t*0.115f+50.0f) + cosf(t*0.120f+200.0f) - sinf(100.0f+t*0.132f))/2.0f;
            if (alphaVal < 0.20f) {
                alphaVal = 0.20f;
            }
        }
    }

    printf("\n-----\nDone.\n");

    closeAudio();
    UnloadRenderTexture(rtCurrent);
    UnloadRenderTexture(rtTarget);
    for (auto &tex : overlays) {
        UnloadTexture(tex);
    }
    overlays.clear();
    UnloadTexture(tex_noise);
    UnloadTexture(tex_noise2);
    UnloadTexture(tex_tex06);
    UnloadTexture(tex_tex07);
    for (auto *viz : visualizers) {
        viz->unload();
    }
    visualizers.clear();
    overlayImage->unload();
    delete overlayImage;
    overlayImage = NULL;
    delete det_low;
    delete det_high;
    delete vu;
    delete contest;
    det_low = NULL;
    det_high = NULL;
    det = NULL;
    vu = NULL;
    contest = NULL;

    return toMenu;
}

int main(int argc, char * argv[])
{
    int monitorNum = 0;
    int ovrWidth = 0;
    int ovrHeight = 0;

    if (argc > 1) {
        if (argc >= 2) {
            sscanf(argv[1], "%d", &monitorNum);
        }
        if (argc >= 4) {
            sscanf(argv[2], "%d", &ovrWidth);
            sscanf(argv[3], "%d", &ovrHeight);
        }
        if (argc >= 5) {
            sscanf(argv[4], "%f", &autoTimerLimit);
        }
    }

    vector<AudioDevInfo> audioDevs = listInputDevices();
    printf("Available input devices are:\n");
    for (auto &d : audioDevs) {
        printf("**%s** (%u in)\n", d.name.c_str(), d.inChannels);
    }

    SetupChoice choice;
    if (monitorNum > 0) {
        choice.monitor = monitorNum - 1;
    }
    if (ovrWidth && ovrHeight) {
        choice.width = ovrWidth;
        choice.height = ovrHeight;
        choice.resPreset = -1;
        for (int i = 0; i < numResPresets; i++) {
            if (resPresets[i][0] == ovrWidth && resPresets[i][1] == ovrHeight) {
                choice.resPreset = i;
                break;
            }
        }
    }
    if (!audioDevs.empty()) {
        choice.audioDevice = audioDevs[0].id;
        for (int i = 0; i < (int)audioDevs.size(); i++) {
            if (audioDevs[i].id == (unsigned int)RtAudio().getDefaultInputDevice()) {
                choice.audioIndex = i;
                choice.audioDevice = audioDevs[i].id;
                break;
            }
        }
    }

    srand((unsigned int)time(NULL));

    InitWindow(DIALOG_W, DIALOG_H, "CubicFX Setup");
    SetTargetFPS(60);
    SetExitKey(0);

    if (!runSetupDialog(choice, audioDevs)) {
        CloseWindow();
        return 0;
    }

    while (runVisualizer(choice)) {
        if (IsWindowFullscreen()) {
            ToggleFullscreen();
        }
        SetWindowSize(DIALOG_W, DIALOG_H);
        SetWindowMonitor(choice.monitor);
        SetWindowTitle("CubicFX Setup");
        if (!runSetupDialog(choice, audioDevs)) {
            break;
        }
    }

    printf("\n-----\nDone.\n");
    CloseWindow();

    return 0;
}
