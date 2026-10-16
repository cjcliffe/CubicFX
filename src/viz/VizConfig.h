// Copyright (c) 2013 Charles J. Cliffe. All rights reserved.

#pragma once

#include <map>
#include <string>
#include <vector>

// Per-shader configuration carried in an embedded JSON front-matter block
// inside the .fs source itself:
//
//   // @viz-begin
//   // {
//   //   "name": "Voxel Edges",
//   //   "tags": ["voxel"],
//   //   "textures": { "tex0": "png/tex_noise2.png" }
//   // }
//   // @viz-end
//
// Only "textures" affects rendering today; every other field is metadata for
// the browser/random picker and is optional. Unknown fields are ignored so
// the schema can grow without breaking older builds. A shader with no block
// (or an unreadable one) gets defaults derived from its file name.
struct VizConfig {
    int version = 1;
    std::string name;
    std::string description;
    std::string author;
    std::vector<std::string> tags;
    // Sampler uniform name -> image asset path (relative to working dir).
    std::map<std::string, std::string> textures;
    // Relative frequency for random picks.
    double weight = 1.0;
    // True keeps the viz selectable but skips it in auto-advance cycling.
    bool excludeFromAuto = false;
};

VizConfig vizConfigDefaults(const std::string &fragPath);

// Never throws: returns defaults when the block is missing or invalid.
VizConfig parseVizFrontMatter(const std::string &fileText, const std::string &fragPath);
