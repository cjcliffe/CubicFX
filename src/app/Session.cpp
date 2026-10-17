#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <vector>

#include <raylib.h>

#include "viz/Transition.h"
#include "viz/Visualizer.h"
#include "viz/VizConfig.h"
#include "audio/AudioCapture.h"
#include "beat/BeatEngine.h"
#include "ui/SetupDialog.h"

using namespace std;

// ---- Visualizer control ----

Visualizer *currentViz = NULL;
vector<Texture2D> overlays;

struct VizEntry {
    std::unique_ptr<Visualizer> viz;
    VizConfig config;
    std::string frag;
};

static std::vector<VizEntry> roster;
static std::map<std::string, Texture2D> texCache;
static std::unique_ptr<BeatEngine> beat;

// Overlay images, flipped on load. Images are stored top-row-first; flip so
// bottom-left-origin UVs (gl_FragCoord / resolution) sample them upright,
// matching the original.
static Texture2D loadImageFlipped(const char *path) {
    Image img = LoadImage(path);
    ImageFlipVertical(&img);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// Overlay slots bound to Shift+1..9. The overlay/ folder is scanned at
// session start and the first files in natural-sort order fill the slots;
// missing slots stay inactive.
static const int maxOverlays = 9;

// Natural order so numeric prefixes sort as expected (2 before 10).
// Comparison is case-insensitive on non-digit runs.
static bool naturalLess(const std::string &a, const std::string &b) {
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (isdigit((unsigned char)a[i]) && isdigit((unsigned char)b[j])) {
            size_t e = i, f = j;
            while (e < a.size() && isdigit((unsigned char)a[e])) {
                e++;
            }
            while (f < b.size() && isdigit((unsigned char)b[f])) {
                f++;
            }
            std::string na = a.substr(i, e - i), nb = b.substr(j, f - j);
            size_t pa = na.find_first_not_of('0'), pb = nb.find_first_not_of('0');
            std::string ta = (pa == std::string::npos) ? "" : na.substr(pa);
            std::string tb = (pb == std::string::npos) ? "" : nb.substr(pb);
            if (ta.size() != tb.size()) {
                return ta.size() < tb.size();
            }
            if (ta != tb) {
                return ta < tb;
            }
            i = e;
            j = f;
        } else {
            char ca = (char)tolower((unsigned char)a[i]);
            char cb = (char)tolower((unsigned char)b[j]);
            if (ca != cb) {
                return ca < cb;
            }
            i++;
            j++;
        }
    }
    return a.size() < b.size();
}

static bool isOverlayImage(const std::filesystem::path &p) {
    static const char *exts[] = { ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tga", ".qoi", ".dds", ".hdr" };
    std::string ext = p.extension().string();
    for (char &c : ext) {
        c = (char)tolower((unsigned char)c);
    }
    for (auto *e : exts) {
        if (ext == e) {
            return true;
        }
    }
    return false;
}

static void loadOverlays() {
    std::vector<std::string> files;
    std::error_code ec;
    std::filesystem::directory_iterator it("overlay", ec);
    if (ec) {
        printf("loadOverlays: cannot scan overlay/ (%s)\n", ec.message().c_str());
        return;
    }
    for (auto &entry : it) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (!isOverlayImage(entry.path())) {
            continue;
        }
        files.push_back(entry.path().generic_string());
    }
    std::sort(files.begin(), files.end(), naturalLess);
    for (auto &f : files) {
        if ((int)overlays.size() >= maxOverlays) {
            break;
        }
        overlays.push_back(loadImageFlipped(f.c_str()));
    }
    printf("loadOverlays: %d overlay images\n", (int)overlays.size());
}

static std::unique_ptr<Visualizer> overlayImage;
Visualizer *targetViz = NULL;

TransitionState transition;

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

// Last time the target viz changed; drives the now-playing label fade.
static double lastVizChange = -1e9;

// Index of a viz in the roster, or -1.
static void transRandom();
static int rosterIndexOf(Visualizer *viz) {
    for (int i = 0; i < (int)roster.size(); i++) {
        if (roster[i].viz.get() == viz) {
            return i;
        }
    }
    return -1;
}

// Select roster entry i (wrapping). Locking sets manual override, which stops
// auto-advance until Space resumes random cycling.
static void pickIndex(int i, bool lock) {
    if (roster.empty()) {
        return;
    }
    int n = (int)roster.size();
    i = ((i % n) + n) % n;
    targetViz = roster[i].viz.get();
    lastVizChange = GetTime();
    if (lock) {
        manualOverride = true;
        transRandom();
    }
}

// Weighted random pick over the whole roster. Seeds currentViz on first call.
static void vizRandomPick() {
    if (roster.empty()) {
        return;
    }
    double total = 0.0;
    for (auto &e : roster) {
        total += e.config.weight;
    }
    if (total <= 0.0) {
        return;
    }
    double r = ((double)rand() / (double)RAND_MAX) * total;
    Visualizer *picked = roster.front().viz.get();
    for (auto &e : roster) {
        r -= e.config.weight;
        if (r <= 0.0) {
            picked = e.viz.get();
            break;
        }
    }
    targetViz = picked;
    lastVizChange = GetTime();
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

// ---- Shader browser ----
// Toggle with F or / while no Shift held. Typing filters the roster by name,
// file, or tag; Up/Down moves, Enter locks in the highlighted viz (manual
// override), Space resumes random cycling, Esc closes without changing.

static bool browserOpen = false;
static std::string browserQuery;
static int browserSel = 0;
static int browserTop = 0;
static std::vector<int> browserHits;

static const int browserPage = 12;

static std::string lowerOf(const std::string &s) {
    std::string out = s;
    for (char &c : out) {
        c = (char)tolower((unsigned char)c);
    }
    return out;
}

static void browserRefresh() {
    browserHits.clear();
    std::string q = lowerOf(browserQuery);
    for (int i = 0; i < (int)roster.size(); i++) {
        std::string hay = lowerOf(roster[i].config.name + " " + roster[i].frag);
        for (auto &t : roster[i].config.tags) {
            hay += " " + lowerOf(t);
        }
        if (q.empty() || hay.find(q) != std::string::npos) {
            browserHits.push_back(i);
        }
    }
    if (browserSel >= (int)browserHits.size()) {
        browserSel = (int)browserHits.size() - 1;
    }
    if (browserSel < 0) {
        browserSel = 0;
    }
    int maxTop = (int)browserHits.size() - browserPage;
    if (maxTop < 0) {
        maxTop = 0;
    }
    if (browserTop > maxTop) {
        browserTop = maxTop;
    }
}

static void browserSetOpen(bool open) {
    browserOpen = open;
    if (open) {
        browserQuery.clear();
        browserSel = 0;
        browserTop = 0;
        browserRefresh();
    }
}

// Keep the highlight inside the visible window, moving the window only when
// the highlight would leave it.
static void browserEnsureVisible() {
    if (browserSel < browserTop) {
        browserTop = browserSel;
    } else if (browserSel >= browserTop + browserPage) {
        browserTop = browserSel - browserPage + 1;
    }
    if (browserTop < 0) {
        browserTop = 0;
    }
    int maxTop = (int)browserHits.size() - browserPage;
    if (maxTop < 0) {
        maxTop = 0;
    }
    if (browserTop > maxTop) {
        browserTop = maxTop;
    }
}

static void browserType(int codepoint) {
    if (codepoint < 32 || codepoint >= 127 || codepoint == ' ') {
        return;
    }
    browserQuery += (char)codepoint;
    browserSel = 0;
    browserTop = 0;
    browserRefresh();
}

struct BrowserLayout {
    int px, py, pw, ph;
    int rowY, rowH;
    int rows, start;
};

// Panel geometry shared by drawBrowser() and click hit-testing. Always
// reserves at least one row so the "(no match)" line never collides with
// the footer hints.
static BrowserLayout browserLayout() {
    BrowserLayout L;
    int scrW = GetScreenWidth();
    int scrH = GetScreenHeight();
    int count = (int)browserHits.size();
    L.rows = count > browserPage ? browserPage : count;
    if (L.rows < 1) {
        L.rows = 1;
    }
    L.rowH = 22;
    L.pw = scrW - 80;
    if (L.pw > 560) {
        L.pw = 560;
    }
    if (L.pw < 200) {
        L.pw = scrW - 20;
    }
    L.px = 20;
    L.py = 20;
    L.ph = 96 + L.rows * L.rowH;
    if (L.ph > scrH - 40) {
        L.ph = scrH - 40;
    }
    L.rowY = L.py + 64;
    L.start = browserTop;
    int maxStart = count - L.rows;
    if (maxStart < 0) {
        maxStart = 0;
    }
    if (L.start > maxStart) {
        L.start = maxStart;
    }
    return L;
}

// Poll this frame's input. With Shift: overlay image select (1-9, keys past
// the loaded images stay inactive) and overlay toggle (0), re-pressing the
// same image reverses its fade.
// Otherwise: Left/Right for prev/next viz, F or / for the shader browser,
// Space for random viz+transition (clears manual override), PageUp/PageDown
// for the beat-detector range. Any direct viz choice sets manual override, which
// stops auto-advance.
static void pollKeys() {
    if (shiftHeld()) {
        static const int overlayKeys[maxOverlays] = {
            KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE,
            KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE
        };
        bool doChange = false;
        thisImage = -1;
        for (int k = 0; k < maxOverlays; k++) {
            if (k < (int)overlays.size() && IsKeyPressed(overlayKeys[k])) {
                overlayImage->setNamedTexture("overlayImage", overlays[k]);
                thisImage = k;
                doChange = true;
            }
        }
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
    } else if (browserOpen) {
        int cp;
        while ((cp = GetCharPressed()) != 0) {
            browserType(cp);
        }
        // NOTE: IsKeyPressedRepeat alone misses single taps (it only fires
        // while the key is held across frames), so always OR with IsKeyPressed.
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && !browserQuery.empty()) {
            browserQuery.pop_back();
            browserSel = 0;
            browserTop = 0;
            browserRefresh();
        }
        if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
            browserSel--;
            if (browserSel < 0 && !browserHits.empty()) {
                browserSel = (int)browserHits.size() - 1;
            }
            browserEnsureVisible();
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
            if (!browserHits.empty()) {
                browserSel = (browserSel + 1) % (int)browserHits.size();
            }
            browserEnsureVisible();
        }
        if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_RIGHT)) && !browserHits.empty()) {
            pickIndex(browserHits[browserSel], true);
            autoTimer = 0;
            browserSetOpen(false);
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            BrowserLayout L = browserLayout();
            Vector2 mp = GetMousePosition();
            for (int r = 0; r < L.rows; r++) {
                int hi = L.start + r;
                if (hi >= (int)browserHits.size()) {
                    break;
                }
                float ry = (float)(L.rowY + r * L.rowH);
                if (mp.x >= (float)L.px && mp.x <= (float)(L.px + L.pw) && mp.y >= ry && mp.y < ry + (float)L.rowH) {
                    pickIndex(browserHits[hi], true);
                    autoTimer = 0;
                    browserSetOpen(false);
                    break;
                }
            }
        }
        if (IsKeyPressed(KEY_SPACE)) {
            browserSetOpen(false);
            vizRandomPick();
            transRandom();
            manualOverride = false;
            autoTimer = 0;
        }
        // Esc while open is handled by the frame loop (closes the browser
        // instead of leaving to the setup menu).
    } else {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
            pickIndex(rosterIndexOf(targetViz) - 1, true);
            autoTimer = 0;
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
            pickIndex(rosterIndexOf(targetViz) + 1, true);
            autoTimer = 0;
        }
        if (IsKeyPressed(KEY_F) || IsKeyPressed(KEY_SLASH)) {
            browserSetOpen(true);
        }
        if (IsKeyPressed(KEY_SPACE)) {
            vizRandomPick();
            transRandom();
            manualOverride = false;
            autoTimer = 0;
        }
        if (IsKeyPressed(KEY_PAGE_DOWN)) {
            printf("Low mode\n");
            beat->det = beat->det_low.get();
        }
        if (IsKeyPressed(KEY_PAGE_UP)) {
            printf("High mode\n");
            beat->det = beat->det_high.get();
        }
    }
}

// ---- Frame rendering ----

// Shared flipped-image cache so shaders referencing the same asset share one
// GPU texture. Keys are resolved asset paths. Any image format raylib can
// load may be used. Bilinear filtering matters: noise LUT shaders sample
// neighboring texels and expect smooth interpolation, but raylib loads
// point-sampled by default.
static Texture2D cachedImage(const std::string &path) {
    auto it = texCache.find(path);
    if (it != texCache.end()) {
        return it->second;
    }
    Texture2D tex = loadImageFlipped(path.c_str());
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    texCache[path] = tex;
    return tex;
}

// Scan shaders/visuals/ recursively and build the roster in sorted order.
// Each collection (shadertoy, glslsandbox, ...) is a subfolder; dropping a .fs
// anywhere under visuals/ adds it to the roster. System shaders (compositor
// effects like the image overlay) live in shaders/system/ and are never
// roster entries. Each shader reads an optional sidecar .json matching the
// .fs name; shaders without one use defaults. Bare texture file names in the
// config resolve to the shader's folder first, then the shared image folder,
// so a shader can use a shared image or ship its own alongside the .fs.
static void buildRoster() {
    currentViz = NULL;
    targetViz = NULL;
    roster.clear();

    std::vector<std::string> frags;
    std::error_code ec;
    std::filesystem::recursive_directory_iterator it("shaders/visuals", ec);
    if (ec) {
        printf("buildRoster: cannot scan shaders/visuals (%s)\n", ec.message().c_str());
        return;
    }
    for (auto &entry : it) {
        if (!entry.is_regular_file()) {
            continue;
        }
        auto path = entry.path();
        if (path.extension() != ".fs") {
            continue;
        }
        frags.push_back(path.generic_string());
    }
    std::sort(frags.begin(), frags.end());

    for (auto &frag : frags) {
        VizEntry e;
        e.frag = frag;
        e.config = loadVizConfig(frag);
        e.viz = std::make_unique<Visualizer>("shaders/vertex_common.vs", frag);
        for (auto &kv : e.config.textures) {
            if (kv.first == "samplerTex") {
                continue;
            }
            e.viz->setNamedTexture(kv.first, cachedImage(resolveVizTexturePath(frag, kv.second)));
        }
        roster.push_back(std::move(e));
    }
    printf("buildRoster: %d visualizers\n", (int)roster.size());
}

static void drawVizInto(RenderTexture2D rt, Visualizer *viz, float t, float dt) {
    viz->updateVariables(t, dt, sample_data, beat->vu->vu_levels, beat->contest.get());
    if (viz->hasBackbuffer) {
        viz->renderFeedback();
        BeginTextureMode(rt);
        ClearBackground(BLANK);
        Rectangle fbSource = { 0, 0, (float)VIZ_WIDTH, -(float)VIZ_HEIGHT };
        Rectangle fbDest = { 0, 0, (float)VIZ_WIDTH, (float)VIZ_HEIGHT };
        DrawTexturePro(viz->feedbackTexture(), fbSource, fbDest, Vector2{ 0, 0 }, 0.0f, WHITE);
        EndTextureMode();
        return;
    }
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

// ---- Browser overlay drawing ----

static std::string targetLabel() {
    int i = rosterIndexOf(targetViz);
    if (i < 0) {
        return "";
    }
    std::string label = roster[i].config.name;
    if (!roster[i].config.author.empty()) {
        label += " by " + roster[i].config.author;
    }
    return label;
}

static void drawBrowser() {
    if (!browserOpen) {
        return;
    }
    BrowserLayout L = browserLayout();
    DrawRectangle(L.px, L.py, L.pw, L.ph, ColorAlpha(BLACK, 0.85f));
    DrawRectangleLines(L.px, L.py, L.pw, L.ph, ColorAlpha(WHITE, 0.4f));
    DrawText("Shader browser", L.px + 12, L.py + 10, 20, WHITE);
    std::string query = std::string("> ") + browserQuery + "_";
    DrawText(query.c_str(), L.px + 12, L.py + 36, 20, YELLOW);
    if (browserHits.empty()) {
        DrawText("(no match)", L.px + 12, L.rowY, 20, GRAY);
    }
    for (int r = 0; r < L.rows; r++) {
        int idx = L.start + r;
        if (idx >= (int)browserHits.size()) {
            break;
        }
        bool sel = (L.start + r) == browserSel;
        Color col = sel ? YELLOW : ColorAlpha(WHITE, 0.85f);
        std::string label = roster[browserHits[idx]].config.name;
        if (label.size() > 48) {
            label = label.substr(0, 48);
        }
        DrawText(label.c_str(), L.px + 12, L.rowY + r * L.rowH, 20, col);
    }
    DrawText("Enter/click lock  Space random  Esc close", L.px + 12, L.py + L.ph - 24, 16, GRAY);
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
    transition = TransitionState();
    lastImage = -1;
    thisImage = 0;
    overlayAlpha = 0.0f;
    overlayDelta = 0.0f;
    overlayEnabled = false;
    autoTimer = 0;
    manualOverride = false;
    browserSetOpen(false);
    audioResetSession(choice.agc, choice.testTone);

    applyWindowMode(choice);

    beat = std::make_unique<BeatEngine>();

    buildRoster();
    if (roster.empty()) {
        printf("No shaders found in shaders/visuals; returning to setup.\n");
        beat.reset();
        return true;
    }

    overlayImage = std::make_unique<Visualizer>("shaders/vertex_common.vs", "shaders/system/overlay_image.fs");

    loadOverlays();

    if (choice.testTone) {
        printf("Using synthesized test tone input.\n");
    } else if (!openAudio(choice.audioDevice)) {
        printf("Continuing without audio input.\n");
    }

    vizRandomPick();
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
            if (browserOpen) {
                browserSetOpen(false);
            } else {
                toMenu = true;
            }
        }

        // Advance an active crossfade/wipe/zoom; a finished transition hands
        // currentViz over to the target.
        advanceTransition(transition, currentViz, targetViz, dt);

        if (transition.inTransition) {
            currentViz->setBlendAlpha(1.0f - transition.timer);
            drawVizInto(rtCurrent, currentViz, t, dt);

            targetViz->setBlendAlpha(transition.timer);
            drawVizInto(rtTarget, targetViz, t, dt);
        }
        else {
            currentViz->setBlendAlpha(1.0f);
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

        if (transition.inTransition) {
            float k = transition.timer;
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

        if (!browserOpen) {
            float age = (float)(GetTime() - lastVizChange);
            if (age < 5.0f) {
                float a = 0.7f * (age < 4.0f ? 1.0f : (5.0f - age));
                std::string label = targetLabel();
                DrawText(label.c_str(), 14, (int)scrH - 26, 20, ColorAlpha(BLACK, a));
                DrawText(label.c_str(), 12, (int)scrH - 28, 20, ColorAlpha(WHITE, a));
            }
        }
        drawBrowser();

        EndDrawing();

        if (!transition.inTransition) autoTimer += dt;
        if (autoTimer > autoTimerLimit && !manualOverride) {
            autoTimer = 0;
            vizRandomPick();
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
    for (auto &kv : texCache) {
        UnloadTexture(kv.second);
    }
    texCache.clear();
    for (auto &e : roster) {
        e.viz->unload();
    }
    roster.clear();
    overlayImage->unload();
    overlayImage.reset();
    beat.reset();

    return toMenu;
}

