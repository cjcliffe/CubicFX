# CubicFX
BeatDetektor / raylib standalone OpenGL shader based music visualizer

[![CircleCI](https://dl.circleci.com/status-badge/img/gh/cjcliffe/CubicFX/tree/master.svg?style=svg)](https://dl.circleci.com/status-badge/redirect/gh/cjcliffe/CubicFX/tree/master)

![CubicFX inversion machine visualizer](docs/screenshot.png)

This was a minimal app created for a few select parties. It was originally built
on CubicVR2/GLFW/OpenAL and has since been ported to raylib with RtAudio capture.
Recent development assistance by Muse Spark 1.3.

Many shaders are derivatives of ShaderToy or the old glsl.heroku site and may not yet contain proper attribution.

## Requirements

- CMake 3.16 or newer
- A C++17 compiler (Visual Studio 2022 on Windows)
- Internet access at configure time — raylib, RtAudio, Dear ImGui, and rlImGui are fetched automatically
- An audio input device (microphone, line-in, or "stereo mix") for the visuals to react to

## Build

```bat
cmake -S . -B build
cmake --build build --config Release
```

Pinned dependencies (see `CMakeLists.txt`): raylib 6.0, RtAudio 6.0.1,
nlohmann/json 3.11.3, Dear ImGui 1.92.7, and rlImGui (Raylib_6_0 branch),
fetched with shallow clones. No vendored libraries or external checkouts required.

## Tests

```bat
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Run

Launch `CubicFX` from the build directory so it finds its `shaders/`, `images/`,
and `fonts/` asset folders, e.g. from `build/`:

```bat
CubicFX.exe
```

A setup dialog appears first — pick options, then press Start (or Enter).
`Esc` in the dialog quits.

![Setup dialog](docs/screenshot_setup.png)

### Setup options

- **Display monitor** — which screen the visualizer opens on
- **Mode** — windowed or fullscreen, plus a **VSync** checkbox beside the
  title to toggle vertical sync (on by default)
- **Resolution** — render resolution in both modes; fullscreen upscales it to
  the monitor's native resolution, so a lower choice runs faster
- **Audio input** — capture device list (mouse wheel or scrollbar to scroll,
  `Refresh` to re-enumerate), plus:
  - **Test tone** — synthesized signal instead of the microphone, useful for
    checking that the visuals react without needing live audio
  - **Auto Gain** — automatic input leveling for quiet sources; uncheck to use
    the raw input signal

## Controls

- `Left` / `Right` — previous/next visualizer (locks in the choice, stopping auto-advance)
- `F` or `/` — shader browser: type to filter by name, file, or tag; `Up`/`Down` moves; `Enter`, `Right`, or click locks in the highlight; `Esc` closes
- `Space` — random visualizer and transition, resumes auto-advance cycling
- `Shift` + `1`–`9` — overlay image select, `Shift` + `0` — toggle overlay
- `PageUp` / `PageDown` (browser closed) — beat detector high/low range mode
- Double-click — toggle windowed/fullscreen
- `Esc` — close browser if open, else back to setup (quit from the setup dialog)

## License

MIT, see `LICENSE`. `BeatDetektor.h` / `BeatDetektor.cpp` are GPLv3.
