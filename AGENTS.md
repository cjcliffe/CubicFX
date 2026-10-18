# Agent Instructions

This is a C++ OpenGL shader-based music visualizer (CubicFX, CMake project/executable name `CubicFX`) built with raylib, RtAudio audio capture, FFT analysis, and BeatDetektor beat detection.

## Build

CMake-based (minimum 3.16, C++17). See `CMakeLists.txt` for build configuration.

- Dependencies are fetched at configure time via `FetchContent` (shallow clones): raylib 6.0 for window/input/rendering, Dear ImGui 1.92.7 + rlImGui (Raylib_6_0 branch) for the setup dialog and RtAudio 6.0.1 for audio input (WASAPI on Windows). No vendored libraries or external checkouts
- Runtime assets are copied to the build directory: `shaders/` and `images/` — run the executable from the build directory so relative asset paths resolve
- On MSVC the target defines `_USE_MATH_DEFINES`; `CubicFX.cpp` defines `NOMINMAX`/`NOGDI`/`NOUSER` before including Windows headers (via RtAudio) to avoid collisions with raylib names like `Rectangle`

## Key Directories

- `src/` — Application source: `main.cpp` (entry, setup/session loop), `app/` (visualizer session: roster, keyboard control, render-texture transitions), `audio/` (RtAudio capture handed to the render thread under a mutex, `FFT.h`), `beat/` (beat-detection glue plus `BeatDetektor.*`), `ui/` (ImGui setup dialog and theme), `viz/` (`Visualizer.cpp/.h`, raylib shader wrapper and per-frame uniforms)
- `shaders/` — Shared `vertex_common.vs` (raylib-style, transforms by the `mvp` uniform); `visuals/` holds roster shaders in per-collection subfolders with sidecar `.json` config; `system/` holds compositor effects; `unused/` stages unadapted shaders. Audio-waveform samplers are `sampler2D` backed by a 1024x1 float texture, not `sampler1D`
- `images/` — Shared noise/texture inputs loaded at runtime
- `overlay/` — Overlay images; the first 9 files in natural-sort order fill the Shift+1..9 slots
- `tools/shadersnap.cpp` — ShaderSnap helper: renders one frame of a visualizer shader offscreen to PNG without launching the app (uniforms get fixed dummy values, textures resolve from the sidecar config with noise-image fallback)

## Verification

- Configure, build, and test via CMake: `cmake -S . -B build`, `cmake --build build --config Release`, `ctest --test-dir build -C Release`
- Snapshot a shader from the build directory (binaries land at the build root, and relative asset paths must resolve): `.\tools\ShaderSnap.exe shaders/visuals/shadertoy/circle_pattern.fs snap.png [linear|point] [time]`

## Conventions

- Comment code where it aids understanding; the no-commentary guidance in Documentation Guidelines targets editorial commentary in reverse-engineering documents, not regular code
- License/header facts: most files use `// Copyright (c) 2013 Charles J. Cliffe. All rights reserved.`; `BeatDetektor` files carry their own GPLv3 header. `README.md` states MIT licence intent for the project — do not add SPDX headers or relicense files
- Many shaders are derivatives of ShaderToy or the old glsl.heroku site and may lack attribution (per `README.md`): do not strip existing attributions, and add one when the source is known

## Documentation Guidelines

When creating or updating documentation in `docs/`, avoid brittle content that will drift from the codebase:

- **No specific line numbers** — Reference functions/classes, not line numbers (e.g., "In `CubicFX.cpp`" not "on line 390")
- **No exact counts** — Avoid "58 shader files" or "42 visualizers" — use qualitative terms like "multiple" or "several"
- **No "Last Updated" timestamps** — They create maintenance burden with zero value
- **No redundant preambles** — Don't repeat the project description in every doc when an index already links everything
- **No theoretical discussions** — Keep design considerations in code comments, not implementation plans
- **No editorial commentary** — Describe what the code does, not what it doesn't do. Avoid negative framing ("X is not handled by Y") and notes about what was corrected during editing. The spec should reflect the current state of the code, not the reverse-engineering process
- **Reference functions, not lines** — "In the `pollKeys()` function" is durable; "on line 390" is not
- **Keep actionable content only** — File lists, verification criteria, and rollback strategies are durable; inventories of current state go stale
