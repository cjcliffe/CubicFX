#include <cstdio>
#include <string>
#include <vector>

#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include "Flex.h"
#include "audio/AudioCapture.h"
#include "ui/SetupDialog.h"

using namespace std;

// ---- Setup dialog ----

// Render-resolution presets for one monitor: standard sizes that fit inside
// the current mode (largest first), plus the exact native size when no
// standard size matches it.
static vector<ResPreset> buildResPresets(int monitor) {
    static const int master[][2] = {
        { 3840, 2160 },
        { 2560, 1440 },
        { 1920, 1080 },
        { 1600, 900 },
        { 1366, 768 },
        { 1280, 720 },
        { 1280, 768 },
        { 1024, 768 },
        { 800, 600 },
    };
    int maxW = GetMonitorWidth(monitor);
    int maxH = GetMonitorHeight(monitor);
    vector<ResPreset> list;
    for (size_t k = 0; k < sizeof(master) / sizeof(master[0]); k++) {
        if (master[k][0] <= maxW && master[k][1] <= maxH) {
            list.push_back({ master[k][0], master[k][1] });
        }
    }
    bool hasNative = false;
    for (auto &e : list) {
        if (e.width == maxW && e.height == maxH) {
            hasNative = true;
            break;
        }
    }
    if (!hasNative && maxW > 0 && maxH > 0) {
        size_t pos = 0;
        while (pos < list.size() && (list[pos].width > maxW ||
            (list[pos].width == maxW && list[pos].height > maxH))) {
            pos++;
        }
        list.insert(list.begin() + pos, { maxW, maxH });
    }
    return list;
}

static int findPreset(const vector<ResPreset> &list, int w, int h) {
    for (int i = 0; i < (int)list.size(); i++) {
        if (list[i].width == w && list[i].height == h) {
            return i;
        }
    }
    return -1;
}

// Match width/height back to a preset index (-1 when custom), clamping to
// the largest entry when the choice exceeds the monitor.
static void resolvePresetChoice(SetupChoice &choice, const vector<ResPreset> &list) {
    choice.resPreset = findPreset(list, choice.width, choice.height);
    if (choice.resPreset < 0 && !list.empty() &&
        (choice.width > GetMonitorWidth(choice.monitor) || choice.height > GetMonitorHeight(choice.monitor))) {
        choice.resPreset = 0;
        choice.width = list[0].width;
        choice.height = list[0].height;
    }
}

// Truncate a device/monitor name for fixed-width buttons.
static string shortName(const string &name, size_t maxLen) {
    if (name.size() <= maxLen) {
        return name;
    }
    return name.substr(0, maxLen - 3) + "...";
}

// Setup dialog widgets are drawn with raygui (see runSetupDialog).

// Modal setup loop on the already-open window. Sections top to bottom:
// monitor, windowed/fullscreen, resolution grid (rebuilt per monitor),
// scrollable audio list with test-tone/auto-gain/refresh, Start. Enter
// starts, Esc or window-close cancels (returns false).
// Widgets are raygui controls (GuiLabel/GuiButton/GuiToggleGroup/
// GuiCheckBox/GuiListViewEx).
bool runSetupDialog(SetupChoice &choice, vector<AudioDevInfo> &audioDevs) {
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
    vector<ResPreset> presets = buildResPresets(choice.monitor);
    resolvePresetChoice(choice, presets);

    bool done = false;
    bool cancelled = false;
    int listScroll = 0;
    int listActive = choice.audioIndex;
    int listFocus = -1;
    int modeActive = choice.fullscreen ? 1 : 0;


    while (!WindowShouldClose() && !done && !cancelled) {
        if (IsKeyPressed(KEY_ENTER)) {
            done = true;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            cancelled = true;
        }

        const float margin = 16.0f;
        const float contentW = (float)dlgW - 32.0f;
        const float listBottom = (float)dlgH - 96.0f;

        int shown = monitorCount < 4 ? monitorCount : 4;
        int resRows = ((int)presets.size() + 3) / 4;
        if (resRows < 1) {
            resRows = 1;
        }
        float monitorH = 0.0f;
        if (shown > 0) {
            monitorH = (float)(shown * 30 + (shown - 1) * 8);
        }
        float gridH = 0.0f;
        if (!presets.empty()) {
            gridH = (float)(resRows * 30 + (resRows - 1) * 8);
        }

        Flex col;
        col.dir = FlexDir::Column;
        col.bounds = Rectangle{ margin, 12.0f, contentW, listBottom - 12.0f };
        col.gap = 6.0f;
        col.addFixed(34.0f);
        col.addFixed(24.0f);
        col.addFixed(monitorH);
        col.addFixed(24.0f);
        col.addFixed(30.0f);
        col.addFixed(24.0f);
        col.addFixed(gridH);
        if (choice.fullscreen) {
            col.addFixed(24.0f);
        }
        col.addFixed(26.0f);
        if (audioDevs.empty()) {
            col.addFixed(24.0f);
        } else {
            col.addGrow(1.0f);
        }
        col.layout();

        size_t k = 0;
        BeginDrawing();
        ClearBackground(BLACK);

        GuiSetStyle(DEFAULT, TEXT_SIZE, 28);
        GuiLabel(col.at(k++), "CubicFX Setup");
        GuiSetStyle(DEFAULT, TEXT_SIZE, 20);

        GuiLabel(col.at(k++), "Display monitor:");
        {
            Rectangle r = col.at(k++);
            string group;
            for (int m = 0; m < shown; m++) {
                char label[160];
                snprintf(label, sizeof(label), "%d: %s (%dx%d)", m + 1,
                    shortName(GetMonitorName(m), 34).c_str(),
                    GetMonitorWidth(m), GetMonitorHeight(m));
                if (m > 0) {
                    group += "\n";
                }
                group += label;
            }
            if (!group.empty()) {
                int prevMonitor = choice.monitor;
                if (GuiToggleGroup(Rectangle{ r.x, r.y, r.width, 30.0f }, group.c_str(), &choice.monitor)) {
                    if (choice.monitor < 0 || choice.monitor >= shown) {
                        choice.monitor = prevMonitor;
                    }
                }
                if (choice.monitor != prevMonitor) {
                    presets = buildResPresets(choice.monitor);
                    resolvePresetChoice(choice, presets);
                }
            }
        }

        {
            Rectangle r = col.at(k++);
            GuiLabel(Rectangle{ r.x, r.y, 80.0f, 24.0f }, "Mode:");
            GuiCheckBox(Rectangle{ r.x + 84.0f, r.y + 2.0f, 20.0f, 20.0f }, "VSync", &choice.vsync);
        }
        {
            Rectangle r = col.at(k++);
            float bw = (r.width - 8.0f) / 2.0f;
            modeActive = choice.fullscreen ? 1 : 0;
            if (GuiToggleGroup(Rectangle{ r.x, r.y, bw, 30.0f }, "Windowed;Fullscreen", &modeActive)) {
                choice.fullscreen = (modeActive == 1);
            }
        }

        GuiLabel(col.at(k++), "Resolution:");
        {
            Rectangle r = col.at(k++);
            float bw = (r.width - 3.0f * 8.0f) / 4.0f;
            string group;
            for (int i = 0; i < (int)presets.size(); i++) {
                char label[32];
                snprintf(label, sizeof(label), "%dx%d", presets[i].width, presets[i].height);
                if (i > 0) {
                    group += (i % 4 == 0) ? "\n" : ";";
                }
                group += label;
            }
            if (!group.empty()) {
                int prevPreset = choice.resPreset;
                GuiToggleGroup(Rectangle{ r.x, r.y, bw, 30.0f }, group.c_str(), &choice.resPreset);
                if (choice.resPreset != prevPreset && choice.resPreset >= 0 && choice.resPreset < (int)presets.size()) {
                    choice.width = presets[choice.resPreset].width;
                    choice.height = presets[choice.resPreset].height;
                }
                if (choice.resPreset < -1 || choice.resPreset >= (int)presets.size()) {
                    choice.resPreset = prevPreset;
                }
            }
        }
        if (choice.fullscreen) {
            Rectangle r = col.at(k++);
            char label[160];
            snprintf(label, sizeof(label), "Fullscreen renders chosen res, upscaled to %dx%d",
                GetMonitorWidth(choice.monitor), GetMonitorHeight(choice.monitor));
            GuiLabel(r, label);
        }

        {
            Rectangle r = col.at(k++);
            GuiLabel(Rectangle{ r.x, r.y, 120.0f, 24.0f }, "Audio input:");
            GuiCheckBox(Rectangle{ r.x + 134.0f, r.y + 2.0f, 20.0f, 20.0f }, "Test tone", &choice.testTone);
            GuiCheckBox(Rectangle{ r.x + r.width - 268.0f, r.y + 2.0f, 20.0f, 20.0f }, "Auto Gain", &choice.agc);
            if (GuiButton(Rectangle{ r.x + r.width - 100.0f, r.y, 100.0f, 26.0f }, "Refresh")) {
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
                listActive = choice.audioIndex;
                listScroll = 0;
                listFocus = -1;
            }
        }
        if (audioDevs.empty()) {
            GuiLabel(col.at(k++), "No audio devices found.");
        } else {
            Rectangle panel = col.at(k++);
            vector<string> labels;
            labels.reserve(audioDevs.size());
            for (auto &d : audioDevs) {
                char buf[160];
                if (d.isLoopback) {
                    snprintf(buf, sizeof(buf), "[Desktop] %s (%uch)", shortName(d.name, 32).c_str(), d.inChannels);
                } else {
                    snprintf(buf, sizeof(buf), "%s (%uch)", shortName(d.name, 40).c_str(), d.inChannels);
                }
                labels.push_back(buf);
            }
            vector<char *> ptrs;
            ptrs.reserve(labels.size());
            for (auto &s : labels) {
                ptrs.push_back(s.data());
            }
            if (listActive < 0 || listActive >= (int)ptrs.size()) {
                listActive = choice.audioIndex;
            }
            int prevActive = listActive;
            GuiListViewEx(panel, ptrs.data(), (int)ptrs.size(), &listScroll, &listActive, &listFocus);
            if (listActive != prevActive && listActive >= 0 && listActive < (int)audioDevs.size()) {
                choice.audioIndex = listActive;
                choice.audioDevice = audioDevs[listActive].id;
            }
        }

        if (GuiButton({ 16, (float)dlgH - 52, (float)dlgW - 32, 36 }, "Start")) {
            done = true;
        }
        GuiLabel({ 16, (float)dlgH - 78, 400, 24 }, "Enter = Start    Esc = Quit");

        EndDrawing();
    }

    return done;
}


