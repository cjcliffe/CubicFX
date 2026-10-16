// Copyright (c) 2013 Charles J. Cliffe. All rights reserved.

#pragma once

#include <map>
#include <string>
#include <vector>

// Per-shader configuration from a sidecar JSON file matching the shader name:
//   shaders/visuals/shadertoy/voxel_edges.fs
//   shaders/visuals/shadertoy/voxel_edges.json
//
//   {
//     "name": "Voxel Edges",
//     "tags": ["voxel"],
//     "textures": { "tex0": "tex_noise2.png" }
//   }
//
// Only "textures" affects rendering today; every other field is metadata for
// the browser/random picker and is optional. Unknown fields are ignored so
// the schema can grow without breaking older builds. A shader with no sidecar
// (or an unreadable one) gets defaults derived from its file name.
struct VizConfig {
    int version = 1;
    std::string name;
    std::string description;
    std::string author;
    std::vector<std::string> tags;
    // Sampler uniform name -> image reference. A bare file name resolves to
    // the shader's own folder first, then the shared image folder; a value
    // containing a path separator is used verbatim as a working-dir-relative
    // asset path.
    std::map<std::string, std::string> textures;
    // Relative frequency for random picks.
    double weight = 1.0;
};

VizConfig vizConfigDefaults(const std::string &fragPath);

// Never throws: returns defaults when the sidecar is missing or invalid.
VizConfig loadVizConfig(const std::string &fragPath);

// Parse JSON text into a config; used by loadVizConfig and unit tests.
VizConfig parseVizConfigJson(const std::string &jsonText, const std::string &fragPath);

// Resolve a texture reference to a working-dir-relative asset path: bare
// file names check the shader's folder first, then the shared image folder.
std::string resolveVizTexturePath(const std::string &fragPath, const std::string &ref);
