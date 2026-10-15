# CubicFX
BeatDetektor / raylib standalone OpenGL shader based music visualizer

This was a minimal app created for a few select parties. It was originally built
on CubicVR2/GLFW/OpenAL and has since been ported to raylib with RtAudio capture.

Many shaders are derivatives of ShaderToy or the old glsl.heroku site and may not yet contain proper attribution.

This project will use the MIT licence.

## Requirements

- CMake 3.16 or newer
- A C++17 compiler (Visual Studio 2022 on Windows)
- Internet access at configure time — raylib and RtAudio are fetched automatically
- An audio input device (microphone, line-in, or "stereo mix") for the visuals to react to

## Build

```bat
cmake -S . -B build
cmake --build build --config Release
```

Pinned dependencies (see `CMakeLists.txt`): raylib 6.0 and RtAudio 6.0.1,
fetched with shallow clones. No vendored libraries or external checkouts required.

## Run

Launch `CubicFX` from the build directory so it finds its `shaders/` and `png/`
asset folders, e.g. from `build/`:

```bat
CubicFX.exe
```

A setup dialog appears first: pick the display monitor, windowed or fullscreen
mode, window resolution, and audio input device, then press Start (or Enter).
Command-line arguments can prefill the dialog:

```
CubicFX.exe [monitor] [width height] [autoTimerLimit]
```

- `monitor` — 1-based display number
- `width height` — windowed resolution
- `autoTimerLimit` — seconds before auto-advancing to a random visualizer

## Controls

- Visualizer select — number keys, `Q`–`P`, `A`–`M`, brackets, `;` `'` `-` `=`
- `Space` — random visualizer and transition
- `Shift` + `1`–`6` — overlay image select, `Shift` + `0` — toggle overlay
- `Up` / `Down` — beat detector high/low range mode
- `PageUp` / `PageDown` (or `,` / `.`, `Shift` for coarse) — global alpha
- `Esc` — back to setup (quit from the setup dialog)
