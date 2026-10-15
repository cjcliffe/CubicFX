#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

#include <raylib.h>

#include "viz/Visualizer.h"
#include "audio/AudioCapture.h"
#include "beat/BeatEngine.h"
#include "ui/SetupDialog.h"

using namespace std;

float alphaVal = 1.0;

// ---- Visualizer control ----

Visualizer *currentViz = NULL;
vector<Texture2D> overlays;

struct VizDef {
    int key;
    const char *frag;
    int tex0 = -1;
    int tex1 = -1;
    int tex2 = -1;
};

struct VizEntry {
    int key;
    std::unique_ptr<Visualizer> viz;
};

static const VizDef vizTable[] = {
    { KEY_ONE, "shaders/rgb_flares_in_tunnel.fs" },
    { KEY_TWO, "shaders/sparkling_sine_wave.fs" },
    { KEY_THREE, "shaders/sparkling_blocks.fs" },
    { KEY_FOUR, "shaders/flare_wave.fs" },
    { KEY_FIVE, "shaders/red_blue_crawl_pattern.fs" },
    { KEY_SIX, "shaders/raymarch_tunnel.fs" },
    { KEY_SEVEN, "shaders/discofloor_ceiling.fs" },
    { KEY_EIGHT, "shaders/flare_tunnel.fs" },
    { KEY_NINE, "shaders/raymarch_fractal3d.fs" },
    { KEY_ZERO, "shaders/raymarch_lines_zoom.fs" },
    { KEY_Q, "shaders/hex_twist_field.fs" },
    { KEY_W, "shaders/discoball.fs" },
    { KEY_E, "shaders/cube_array.fs" },
    { KEY_R, "shaders/planed_fly.fs" },
    { KEY_T, "shaders/planed_relief_tun.fs" },
    { KEY_Y, "shaders/planed_square_tun.fs" },
    { KEY_U, "shaders/hex_tunnel.fs" },
    { KEY_I, "shaders/rm_cubescape.fs" },
    { KEY_O, "shaders/a_lot_of_spheres.fs" },
    { KEY_P, "shaders/fractal_gears.fs" },
    { KEY_A, "shaders/inversion_machine.fs" },
    { KEY_S, "shaders/menger_journey.fs" },
    { KEY_D, "shaders/metaballs.fs" },
    { KEY_F, "shaders/particle_tracing.fs" },
    { KEY_G, "shaders/flying_cubes.fs" },
    { KEY_H, "shaders/spaceship.fs" },
    { KEY_J, "shaders/space_rings.fs" },
    { KEY_K, "shaders/hologram.fs" },
    { KEY_L, "shaders/music_ball.fs" },
    { KEY_Z, "shaders/led_spectrum.fs" },
    { KEY_X, "shaders/2dkifs.fs" },
    { KEY_C, "shaders/planed_tunnel_effect.fs" },
    { KEY_V, "shaders/noxious_box.fs" },
    { KEY_B, "shaders/wave_floor.fs" },
    { KEY_N, "shaders/hex_field.fs" },
    { KEY_M, "shaders/space_race.fs" },
    { KEY_LEFT_BRACKET, "shaders/truchet_tentacles.fs" },
    { KEY_RIGHT_BRACKET, "shaders/circle_pattern.fs", 0 },
    { KEY_SEMICOLON, "shaders/fire.fs" },
    { KEY_APOSTROPHE, "shaders/dubstepmyass.fs" },
    { KEY_MINUS, "shaders/voxel_edges.fs", 1, 3, 2 },
    { KEY_EQUAL, "shaders/noise_blur.fs" },
};

static std::vector<VizEntry> roster;
static std::vector<Texture2D> fxTextures;
static std::unique_ptr<BeatEngine> beat;

// Overlay PNGs, in Shift+1..6 order.
// PNGs are stored top-row-first; flip on load so bottom-left-origin UVs
// (gl_FragCoord / resolution) sample them upright, matching the original.
static Texture2D loadPngFlipped(const char *path) {
    Image img = LoadImage(path);
    ImageFlipVertical(&img);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

static void loadOverlays() {
    overlays.push_back(loadPngFlipped("png/OFFSET.png"));
    overlays.push_back(loadPngFlipped("png/JACK.png"));
    overlays.push_back(loadPngFlipped("png/synthamesk.png"));
    overlays.push_back(loadPngFlipped("png/C64.png"));
    overlays.push_back(loadPngFlipped("png/LAFO.png"));
    overlays.push_back(loadPngFlipped("png/LAFOLOGO.png"));
}

static std::unique_ptr<Visualizer> overlayImage;
Visualizer *targetViz = NULL;

float transition_timer = 0.0;
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

// Pick a random visualizer; seeds currentViz on first call.
void vizRandom() {
    if (roster.empty()) {
        return;
    }
    int nextViz = (int)floor(((float)rand() / (float)RAND_MAX)*roster.size());
    if (nextViz > (int)roster.size() - 1) nextViz = (int)roster.size() - 1;
    targetViz = roster[nextViz].viz.get();
    if (currentViz == NULL) {
        currentViz = targetViz;
    }
}

// Pick a random crossfade/wipe/zoom transition.
void transRandom() {
    trans_type = (transitionType)(int)floor(((float)rand() / (float)RAND_MAX)*TRANS_MAX);
}

static bool shiftHeld() {
    return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

// Poll this frame's input. With Shift: overlay image select (1-6) and
// overlay toggle (0), re-pressing the same image reverses its fade.
// Otherwise: visualizer select, Space for random viz+transition (clears
// manual override), Up/Down for the beat-detector range. Any direct viz
// choice sets manual override, which stops auto-advance and idle alpha.
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
        for (auto &e : roster) {
            if (IsKeyPressed(e.key)) {
                targetViz = e.viz.get();
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
            beat->det = beat->det_low.get();
        }
        if (IsKeyPressed(KEY_UP)) {
            printf("High mode\n");
            beat->det = beat->det_high.get();
        }

        if (targetViz != tempViz) {
            manualOverride = true;
            transRandom();
        }
    }
}

// ---- Frame rendering ----

// Upload uniforms and draw one fullscreen viz pass into an offscreen target
// at render size. Texture bindings happen inside display(), after the shader
// is active (see Visualizer::bindTextures).
static void buildRoster() {
    fxTextures.push_back(loadPngFlipped("png/tex_noise1.png"));
    fxTextures.push_back(loadPngFlipped("png/tex_noise2.png"));
    fxTextures.push_back(loadPngFlipped("png/tex_tex06.png"));
    fxTextures.push_back(loadPngFlipped("png/tex_tex07.png"));
    currentViz = NULL;
    targetViz = NULL;
    roster.clear();
    for (auto &d : vizTable) {
        VizEntry e;
        e.key = d.key;
        e.viz = std::make_unique<Visualizer>("shaders/vertex_common.vs", d.frag);
        if (d.tex0 >= 0) {
            e.viz->setTex0Texture(fxTextures[d.tex0]);
        }
        if (d.tex1 >= 0) {
            e.viz->setTex1Texture(fxTextures[d.tex1]);
        }
        if (d.tex2 >= 0) {
            e.viz->setTex2Texture(fxTextures[d.tex2]);
        }
        roster.push_back(std::move(e));
    }
}

static void drawVizInto(RenderTexture2D rt, Visualizer *viz, float t, float dt) {
    viz->updateVariables(t, dt, sample_data, beat->vu->vu_levels, beat->contest.get());
    BeginTextureMode(rt);
    ClearBackground(BLANK);
    viz->display(VIZ_WIDTH, VIZ_HEIGHT);
    EndTextureMode();
}

// Apply a mode choice to the window. Render size (VIZ_*) always follows the
// chosen preset; the window itself is the preset in windowed mode and the
// monitor native mode in fullscreen (render output is upscaled on compose).
static void applyWindowMode(const SetupChoice &choice) {
    if (choice.fullscreen) {
        SetWindowSize(GetMonitorWidth(choice.monitor), GetMonitorHeight(choice.monitor));
        SetWindowMonitor(choice.monitor);
        if (!IsWindowFullscreen()) {
            ToggleFullscreen();
        }
    } else {
        if (IsWindowFullscreen()) {
            ToggleFullscreen();
        }
        SetWindowSize(choice.width, choice.height);
        SetWindowMonitor(choice.monitor);
    }
    VIZ_WIDTH = choice.width;
    VIZ_HEIGHT = choice.height;
    SetWindowTitle("BeatDetektor CubicFX");
}

// One visualizer session: reset shared state, apply the window mode, build
// the viz roster and textures, open audio, then run the frame loop until Esc
// (returns true = back to setup) or window-close (returns false = quit).
// Everything session-local is torn down before returning so sessions restart
// cleanly, including GPU programs via Visualizer::unload.
bool runVisualizer(SetupChoice &choice)
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
    audioResetSession(choice.agc, choice.testTone);

    applyWindowMode(choice);

    beat = std::make_unique<BeatEngine>();

    buildRoster();

    overlayImage = std::make_unique<Visualizer>("shaders/vertex_common.vs", "shaders/overlay_image.fs");

    loadOverlays();

    if (choice.testTone) {
        printf("Using synthesized test tone input.\n");
    } else if (!openAudio(choice.audioDevice)) {
        printf("Continuing without audio input.\n");
    }

    vizRandom();
    transRandom();

    RenderTexture2D rtCurrent = LoadRenderTexture(VIZ_WIDTH, VIZ_HEIGHT);
    RenderTexture2D rtTarget = LoadRenderTexture(VIZ_WIDTH, VIZ_HEIGHT);
    RenderTexture2D rtOverlay = LoadRenderTexture(VIZ_WIDTH, VIZ_HEIGHT);
    Rectangle rtSource = { 0, 0, (float)VIZ_WIDTH, -(float)VIZ_HEIGHT };
    Vector2 rtOrigin = { 0, 0 };

    SetTargetFPS(60);

    bool toMenu = false;
    double lastClickTime = -1.0;
    Vector2 lastClickPos = { 0, 0 };
    while (!WindowShouldClose() && !toMenu)
    {
        float dt = GetFrameTime();
        float t = (float)GetTime();

        captureAudio();
        beat->process(t, dt);

        pollKeys();

        // Double-click (two presses <0.4s apart within 8px) toggles fullscreen.
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            double now = GetTime();
            Vector2 mp = GetMousePosition();
            float dx = mp.x - lastClickPos.x;
            float dy = mp.y - lastClickPos.y;
            if (lastClickTime >= 0.0 && (now - lastClickTime) < 0.4 && (dx*dx + dy*dy) < 64.0f) {
                choice.fullscreen = !choice.fullscreen;
    applyWindowMode(choice);

    if (choice.vsync) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }
                lastClickTime = -1.0;
            } else {
                lastClickTime = now;
                lastClickPos = mp;
            }
        }

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

        // Advance an active crossfade/wipe/zoom; a finished transition hands
        // currentViz over to the target.
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

        bool showOverlay = overlayEnabled || overlayAlpha != 0.0f;
        if (showOverlay) {
            overlayImage->setBlendAlpha(overlayAlpha);
            overlayImage->updateVariables(t, dt, sample_data, beat->vu->vu_levels, beat->contest.get());
            BeginTextureMode(rtOverlay);
            ClearBackground(BLANK);
            overlayImage->display(VIZ_WIDTH, VIZ_HEIGHT);
            EndTextureMode();
        }

        float scrW = (float)GetScreenWidth();
        float scrH = (float)GetScreenHeight();
        Rectangle screenDest = { 0, 0, scrW, scrH };

        BeginDrawing();
        ClearBackground(BLACK);

        if (in_transition) {
            float k = transition_timer;
            switch (trans_type) {
            case TRANS_CROSSFADE:
                DrawTexturePro(rtCurrent.texture, rtSource, screenDest, rtOrigin, 0.0f, WHITE);
                DrawTexturePro(rtTarget.texture, rtSource, screenDest, rtOrigin, 0.0f, ColorAlpha(WHITE, k));
                break;
            case TRANS_WIPERIGHT:
                DrawTexturePro(rtCurrent.texture, rtSource, screenDest, rtOrigin, 0.0f, WHITE);
                BeginScissorMode(0, 0, (int)(k*scrW), (int)scrH);
                DrawTexturePro(rtTarget.texture, rtSource, screenDest, rtOrigin, 0.0f, WHITE);
                EndScissorMode();
                break;
            case TRANS_WIPELEFT:
                DrawTexturePro(rtCurrent.texture, rtSource, screenDest, rtOrigin, 0.0f, WHITE);
                BeginScissorMode((int)(scrW - k*scrW), 0, (int)(k*scrW), (int)scrH);
                DrawTexturePro(rtTarget.texture, rtSource, screenDest, rtOrigin, 0.0f, WHITE);
                EndScissorMode();
                break;
            case TRANS_WIPEDOWN:
                DrawTexturePro(rtCurrent.texture, rtSource, screenDest, rtOrigin, 0.0f, WHITE);
                BeginScissorMode(0, 0, (int)scrW, (int)(k*scrH));
                DrawTexturePro(rtTarget.texture, rtSource, screenDest, rtOrigin, 0.0f, WHITE);
                EndScissorMode();
                break;
            case TRANS_WIPEUP:
                DrawTexturePro(rtCurrent.texture, rtSource, screenDest, rtOrigin, 0.0f, WHITE);
                BeginScissorMode(0, (int)(scrH - k*scrH), (int)scrW, (int)(k*scrH));
                DrawTexturePro(rtTarget.texture, rtSource, screenDest, rtOrigin, 0.0f, WHITE);
                EndScissorMode();
                break;
            case TRANS_ZOOMIN: {
                DrawTexturePro(rtCurrent.texture, rtSource, screenDest, rtOrigin, 0.0f, WHITE);
                float zw = k*scrW;
                float zh = k*scrH;
                Rectangle zoomDest = { (scrW - zw) / 2.0f, (scrH - zh) / 2.0f, zw, zh };
                DrawTexturePro(rtTarget.texture, rtSource, zoomDest, rtOrigin, 0.0f, WHITE);
                break;
            }
            default:
                DrawTexturePro(rtCurrent.texture, rtSource, screenDest, rtOrigin, 0.0f, WHITE);
                break;
            }
        }
        else {
            DrawTexturePro(rtCurrent.texture, rtSource, screenDest, rtOrigin, 0.0f, WHITE);
        }

        if (showOverlay) {
            DrawTexturePro(rtOverlay.texture, rtSource, screenDest, rtOrigin, 0.0f, WHITE);
        }

        EndDrawing();

        if (!in_transition) autoTimer += dt;
        if (autoTimer > autoTimerLimit && !manualOverride) {
            autoTimer = 0;
            vizRandom();
        }

        // Idle brightness pulse while no viz was picked manually.
        if (!manualOverride) {
            alphaVal = 0.925f + 0.075f * sinf(t*0.9f);
        }
    }

    printf("\n-----\nDone.\n");

    closeAudio();
    UnloadRenderTexture(rtCurrent);
    UnloadRenderTexture(rtTarget);
    UnloadRenderTexture(rtOverlay);
    for (auto &tex : overlays) {
        UnloadTexture(tex);
    }
    overlays.clear();
    for (auto &tex : fxTextures) {
        UnloadTexture(tex);
    }
    fxTextures.clear();
    for (auto &e : roster) {
        e.viz->unload();
    }
    roster.clear();
    overlayImage->unload();
    overlayImage.reset();
    beat.reset();

    return toMenu;
}

