# Shaders

Each visualizer is a fragment shader under `shaders/visuals/`, paired with
the shared `shaders/vertex_common.vs`. Each collection is a subfolder (one
per source or theme); the roster is built at session start by scanning that
tree recursively, so adding a shader is just dropping a `.fs` file anywhere
under it. Shaders not yet wired to any audio input live in `shaders/unused/`
(mirroring the collection layout) until they are adapted; that folder is
never scanned. Compositor effects that are not visualizers (currently the
image overlay) live in `shaders/system/` and are never scanned into the roster.

## Front-matter

A shader can carry its own configuration in an embedded JSON block at the top
of the file. Shaders without a block use defaults (display name derived from
the file name, no image textures, normal random weight).

```glsl
// @viz-begin
// {
//   "name": "Voxel Edges",
//   "author": "inigo quilez",
//   "tags": ["voxel", "raymarch"],
//   "textures": {
//     "tex0": "png/tex_noise2.png",
//     "tex1": "png/tex_tex07.png",
//     "tex2": "png/tex_tex06.png"
//   },
//   "weight": 1.0,
//   "excludeFromAuto": false
// }
// @viz-end
```

A single-line form (`// @viz { ... }`) also works for small configs.

### Fields

- `name` — display name used by the browser overlay. Defaults to the file
  stem.
- `description`, `author` — informational metadata.
- `tags` — extra search terms for the browser.
- `textures` — sampler uniform name to image asset path. Paths resolve the
  same way as other runtime assets (relative to the working directory the app
  runs from). The waveform `samplerTex` texture is managed by the renderer
  and does not need an entry. Any sampler the shader declares can be bound
  this way, so future samplers need no code change.
- `weight` — relative frequency for random picks. Must be positive.
- `excludeFromAuto` — keeps the shader selectable via browser and prev/next
  but skips it during automatic cycling.
- `version` — schema version, currently informational.

Unknown fields are ignored, so new options can be added without breaking
older builds. An unreadable block logs a warning and falls back to defaults.

## Controls

- Left/Right arrows — previous/next shader (locks in the choice, stopping
  auto-advance).
- F or `/` — open the shader browser. Type to filter by name, file, or tag;
  Up/Down moves; Enter, Right arrow, or click locks in the highlight; Esc closes.
- Space — random shader, resumes automatic cycling.
- Up/Down (browser closed) — beat-detector range.
- Shift+number — overlay image controls, unchanged.
