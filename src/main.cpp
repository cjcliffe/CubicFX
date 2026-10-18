#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>

#include <raylib.h>

#include "rlImGui.h"

#include "audio/AudioCapture.h"
#include "ui/SetupDialog.h"
#include "ui/UiStyle.h"
#include "app/Session.h"

using namespace std;

// Entry point: list audio devices, open the window on the setup dialog,
// then alternate dialog/visualizer sessions until quit. Dear ImGui (via
// rlImGui) is set up once here and only draws inside runSetupDialog; the
// visualizer session renders plain raylib frames.
int main()
{
    vector<AudioDevInfo> audioDevs = listInputDevices();
    printf("Available audio devices are (* = default):\n");
    for (auto &d : audioDevs) {
        bool isDefault = d.isLoopback ? d.isDefaultOutput : d.isDefaultInput;
        printf("**%s** (%u in, %u out)%s%s\n", d.name.c_str(), d.inChannels, d.outChannels,
            d.isLoopback ? " [loopback]" : "", isDefault ? " *" : "");
    }

    SetupChoice choice;
    if (!audioDevs.empty()) {
        choice.audioDevice = audioDevs[0].id;
        choice.audioIndex = 0;
        unsigned int defaultIn = audioDefaultInputDevice();
        unsigned int defaultOut = audioDefaultOutputDevice();
        bool found = false;
        for (int i = 0; i < (int)audioDevs.size(); i++) {
            if (audioDevs[i].id == defaultOut && audioDevs[i].isLoopback) {
                choice.audioIndex = i;
                choice.audioDevice = audioDevs[i].id;
                found = true;
                break;
            }
        }
        if (!found) {
            for (int i = 0; i < (int)audioDevs.size(); i++) {
                if (audioDevs[i].isDefaultOutput && audioDevs[i].isLoopback) {
                    choice.audioIndex = i;
                    choice.audioDevice = audioDevs[i].id;
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            for (int i = 0; i < (int)audioDevs.size(); i++) {
                if (audioDevs[i].id == defaultIn) {
                    choice.audioIndex = i;
                    choice.audioDevice = audioDevs[i].id;
                    found = true;
                    break;
                }
            }
        }
    }

    srand((unsigned int)time(NULL));

    InitWindow(DIALOG_W, DIALOG_H, "CubicFX Setup");
    SetTargetFPS(60);
    SetExitKey(0);

    rlImGuiBeginInitImGui();
    applyUiStyle();
    rlImGuiEndInitImGui();

    while (true) {
        if (!runSetupDialog(choice, audioDevs)) {
            break;
        }
        if (!runVisualizer(choice)) {
            break;
        }
        if (IsWindowFullscreen()) {
            ToggleFullscreen();
        }
        SetWindowSize(DIALOG_W, DIALOG_H);
        SetWindowMonitor(choice.monitor);
        SetWindowTitle("CubicFX Setup");
    }

    printf("\n-----\nDone.\n");
    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
