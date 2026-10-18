#include <cfloat>
#include <cstdio>
#include <string>
#include <vector>

#include <raylib.h>

#include "imgui.h"
#include "rlImGui.h"

#include "audio/AudioCapture.h"
#include "ui/SetupDialog.h"
#include "ui/UiStyle.h"

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

// Full-width toggle button in the old setup style: the active choice renders
// in the sky-blue accent with dark text, the rest as plain buttons.
static bool toggleButton(const char *label, bool active, const ImVec2 &size) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.75f, 1.00f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f, 0.80f, 1.00f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.33f, 0.68f, 0.95f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    }
    bool clicked = ImGui::Button(label, size);
    if (active) {
        ImGui::PopStyleColor(4);
    }
    return clicked;
}

// Modal setup loop on the already-open window, drawn with Dear ImGui through
// the rlImGui backend. Sections top to bottom: monitor buttons, vsync plus
// windowed/fullscreen buttons, resolution button grid (rebuilt per monitor),
// audio options with a scrollable device list, then Start. Enter starts,
// Esc or window-close cancels (returns false).
bool runSetupDialog(SetupChoice &choice, vector<AudioDevInfo> &audioDevs) {
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
    int listActive = choice.audioIndex;

    while (!WindowShouldClose() && !done && !cancelled) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            done = true;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            cancelled = true;
        }

        BeginDrawing();
        ClearBackground(BLACK);
        rlImGuiBegin();

        ImGuiIO &io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::Begin("CubicFX Setup", nullptr, hostFlags)) {
            if (ImFont *title = uiTitleFont()) {
                ImGui::PushFont(title);
            }
            ImGui::Text("CubicFX Setup");
            if (uiTitleFont()) {
                ImGui::PopFont();
            }

            ImGui::SeparatorText("Display monitor");
            for (int m = 0; m < monitorCount; m++) {
                char label[160];
                snprintf(label, sizeof(label), "%d: %s (%dx%d)", m + 1,
                    shortName(GetMonitorName(m), 34).c_str(),
                    GetMonitorWidth(m), GetMonitorHeight(m));
                ImGui::PushID(m);
                if (toggleButton(label, choice.monitor == m, ImVec2(-FLT_MIN, 0.0f))) {
                    choice.monitor = m;
                    presets = buildResPresets(choice.monitor);
                    resolvePresetChoice(choice, presets);
                }
                ImGui::PopID();
            }

            ImGui::SeparatorText("Mode");
            ImGui::Checkbox("VSync", &choice.vsync);
            {
                float half = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.0f;
                if (toggleButton("Windowed", !choice.fullscreen, ImVec2(half, 0.0f))) {
                    choice.fullscreen = false;
                }
                ImGui::SameLine();
                if (toggleButton("Fullscreen", choice.fullscreen, ImVec2(half, 0.0f))) {
                    choice.fullscreen = true;
                }
            }

            ImGui::SeparatorText("Resolution");
            if (ImGui::BeginTable("##res", 4)) {
                for (int i = 0; i < (int)presets.size(); i++) {
                    ImGui::TableNextColumn();
                    char label[32];
                    snprintf(label, sizeof(label), "%dx%d", presets[i].width, presets[i].height);
                    ImGui::PushID(i);
                    if (toggleButton(label, choice.resPreset == i, ImVec2(-FLT_MIN, 0.0f))) {
                        choice.resPreset = i;
                        choice.width = presets[i].width;
                        choice.height = presets[i].height;
                    }
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
            if (choice.fullscreen) {
                ImGui::TextDisabled("Fullscreen renders chosen res, upscaled to %dx%d",
                    GetMonitorWidth(choice.monitor), GetMonitorHeight(choice.monitor));
            }

            ImGui::SeparatorText("Audio input");
            ImGui::Checkbox("Test tone", &choice.testTone);
            ImGui::SameLine();
            ImGui::Checkbox("Auto Gain", &choice.agc);
            ImGui::SameLine(ImGui::GetWindowWidth() - 116.0f);
            if (ImGui::Button("Refresh", ImVec2(100.0f, 0.0f))) {
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
            }

            if (audioDevs.empty()) {
                ImGui::Text("No audio devices found.");
            } else {
                vector<string> labels;
                labels.reserve(audioDevs.size());
                for (auto &d : audioDevs) {
                    char buf[160];
                    bool isDefault = d.isLoopback ? d.isDefaultOutput : d.isDefaultInput;
                    if (d.isLoopback) {
                        snprintf(buf, sizeof(buf), "[Desktop] %s (%uch)%s", shortName(d.name, 32).c_str(),
                            d.inChannels, isDefault ? " *" : "");
                    } else {
                        snprintf(buf, sizeof(buf), "%s (%uch)%s", shortName(d.name, 40).c_str(), d.inChannels,
                            isDefault ? " *" : "");
                    }
                    labels.push_back(buf);
                }
                if (listActive < 0 || listActive >= (int)labels.size()) {
                    listActive = choice.audioIndex;
                }
                // Fill the width and stretch down to the footer, so no dead
                // space remains at the dialog bottom.
                if (ImGui::BeginListBox("##audiodevs", ImVec2(-FLT_MIN, -96.0f))) {
                    for (int i = 0; i < (int)labels.size(); i++) {
                        bool selected = (listActive == i);
                        if (ImGui::Selectable(labels[i].c_str(), selected)) {
                            listActive = i;
                            choice.audioIndex = i;
                            choice.audioDevice = audioDevs[i].id;
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndListBox();
                }
            }

            ImGui::TextDisabled("Enter = Start    Esc = Quit");
            if (ImGui::Button("Start", ImVec2(-FLT_MIN, 36.0f))) {
                done = true;
            }
        }
        ImGui::End();

        rlImGuiEnd();
        EndDrawing();
    }

    return done;
}
