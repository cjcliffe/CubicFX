#include <cmath>
#include <cstdio>
#include <cstdlib>
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
vector<Visualizer *> visualizers;
vector<Texture2D> overlays;

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

Visualizer *overlayImage;
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
    int nextViz = (int)floor(((float)rand() / (float)RAND_MAX)*visualizers.size());
    if (nextViz > (int)visualizers.size() - 1) nextViz = (int)visualizers.size() - 1;
    targetViz = visualizers[nextViz];
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

// Map a pressed key to its visualizer; order matches the push_back sequence.
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

// ---- Frame rendering ----

// Upload uniforms and draw one fullscreen viz pass into an offscreen target
// at render size. Texture bindings happen inside display(), after the shader
// is active (see Visualizer::bindTextures).
static void drawVizInto(RenderTexture2D rt, Visualizer *viz, float t, float dt) {
    viz->updateVariables(t, dt, sample_data, vu->vu_levels, contest);
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

    Texture2D tex_noise = loadPngFlipped("png/tex_noise1.png");
    Texture2D tex_noise2 = loadPngFlipped("png/tex_noise2.png");
    Texture2D tex_tex06 = loadPngFlipped("png/tex_tex06.png");
    Texture2D tex_tex07 = loadPngFlipped("png/tex_tex07.png");

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

    if (choice.testTone) {
        printf("Using synthesized test tone input.\n");
    } else if (!openAudio(choice.audioDevice)) {
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
        processBD(t, dt);

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
            overlayImage->updateVariables(t, dt, sample_data, vu->vu_levels, contest);
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

