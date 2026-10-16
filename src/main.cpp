#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>

#include <raylib.h>

#include "audio/AudioCapture.h"
#include "ui/SetupDialog.h"
#include "ui/UiStyle.h"
#include "app/Session.h"

using namespace std;

// Entry point: parse CLI prefill args, list audio devices, open the window
// on the setup dialog, then alternate dialog/visualizer sessions until quit.
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
    printf("Available audio devices are (* = default):\n");
    for (auto &d : audioDevs) {
        bool isDefault = d.isLoopback ? d.isDefaultOutput : d.isDefaultInput;
        printf("**%s** (%u in, %u out)%s%s\n", d.name.c_str(), d.inChannels, d.outChannels,
            d.isLoopback ? " [loopback]" : "", isDefault ? " *" : "");
    }

    SetupChoice choice;
    if (monitorNum > 0) {
        choice.monitor = monitorNum - 1;
    }
    if (ovrWidth && ovrHeight) {
        choice.width = ovrWidth;
        choice.height = ovrHeight;
        choice.resPreset = -1;
    }
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

    applyUiStyle();

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
    CloseWindow();

    return 0;
}
