# Shaders

Each visualizer is a fragment shader under `shaders/visuals/`, paired with
the shared `shaders/vertex_common.vs`. Each collection is a subfolder (one
per source or theme); the roster is built at session start by scanning that
tree recursively, so adding a shader is just dropping a `.fs` file anywhere
under it. Shaders not yet wired to any audio input live in `shaders/unused/`
(mirroring the collection layout) until they are adapted; that folder is
never scanned. Compositor effects that are not visualizers (currently the
image overlay) live in `shaders/system/` and are never scanned into the roster.

## Sidecar config

A shader can carry its own configuration in a sidecar `.json` file matching
the shader name. Shaders without a sidecar use defaults (display name derived
from the file name, no image textures, normal random weight).

```
shaders/visuals/shadertoy/voxel_edges/voxel_edges.fs
shaders/visuals/shadertoy/voxel_edges/voxel_edges.json
shaders/visuals/shadertoy/voxel_edges/tex_tex06.png
shaders/visuals/shadertoy/voxel_edges/tex_tex07.png
```

```json
{
  "name": "Voxel Edges",
  "author": "inigo quilez",
  "tags": ["voxel", "raymarch"],
  "textures": {
    "tex0": "tex_noise2.png",
    "tex1": "tex_tex07.png",
    "tex2": "tex_tex06.png"
  },
  "weight": 1.0
}
```

### Fields

- `name` — display name used by the browser overlay. Defaults to the file
  stem.
- `description`, `author` — informational metadata.
- `tags` — extra search terms for the browser.
- `textures` — sampler uniform name to image reference. A bare file name
  resolves to the shader's own folder first, then the shared `images/`
  folder, so a shader can use a shared image or ship its own alongside the
  `.fs` (any image format raylib can load). A value containing a path
  separator is used verbatim as a working-dir-relative asset path.
  Textures load vertically flipped (matching the original
  orientation) with bilinear filtering, which noise LUT shaders depend on.
  The waveform `samplerTex` texture is managed by the renderer
  and does not need an entry. Any sampler the shader declares can be bound
  this way, so future samplers need no code change.
- `weight` — relative frequency for random picks. Must be positive.
- `version` — schema version, currently informational.

Unknown fields are ignored, so new options can be added without breaking
older builds. A missing or unreadable sidecar falls back to defaults.

## Frame feedback (backbuffer)

A shader that declares `uniform sampler2D backbuffer` samples the previous
frame it rendered. Each such visual keeps two private render targets and
alternates between them: the last frame is bound to `backbuffer` while the
next frame renders into the other one, so trails and persistence accumulate.
Buffers start cleared and are reallocated when the render size changes. Like
`samplerTex`, `backbuffer` is owned by the renderer and takes no sidecar
`textures` entry; tag feedback shaders with `"feedback"`. ShaderSnap renders
a warm-up sequence for these shaders so snapshots show the accumulated
effect.

## Controls

- Left/Right arrows — previous/next shader (locks in the choice, stopping
  auto-advance).
- F or `/` — open the shader browser. Type to filter by name, file, or tag;
  Up/Down moves; Enter, Right arrow, or click locks in the highlight; Esc closes.
- Space — random shader, resumes automatic cycling.
- PageUp/PageDown (browser closed) — beat-detector range.
- Shift+number — overlay image controls: the first 9 image files in `overlay/`
  (natural-sort order) fill the Shift+1..9 slots; missing slots stay inactive.
