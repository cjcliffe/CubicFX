#include <cstdio>
#include <string>

#include "viz/VizConfig.h"

static int failures = 0;

static void check(bool ok, const char *name) {
    if (!ok) {
        ++failures;
        std::printf("FAIL %s\n", name);
    } else {
        std::printf("ok %s\n", name);
    }
}

int main() {
    {
        VizConfig cfg = parseVizFrontMatter("#version 150\nvoid main() {}\n", "shaders/some_viz.fs");
        check(cfg.name == "some viz", "defaults derive name from file stem");
        check(cfg.textures.empty(), "defaults have no textures");
        check(cfg.weight == 1.0 && !cfg.excludeFromAuto, "defaults weight/auto");
    }
    {
        std::string src =
            "// @viz-begin\n"
            "// {\n"
            "//   \"name\": \"Voxel Edges\",\n"
            "//   \"tags\": [\"voxel\", \"raymarch\"],\n"
            "//   \"textures\": { \"tex0\": \"png/tex_noise2.png\", \"tex1\": \"png/tex_tex07.png\" },\n"
            "//   \"weight\": 2.5,\n"
            "//   \"excludeFromAuto\": true,\n"
            "//   \"futureField\": [1, 2, 3]\n"
            "// }\n"
            "// @viz-end\n"
            "#version 150\n";
        VizConfig cfg = parseVizFrontMatter(src, "shaders/voxel_edges.fs");
        check(cfg.name == "Voxel Edges", "block name");
        check(cfg.tags.size() == 2 && cfg.tags[0] == "voxel", "block tags");
        check(cfg.textures.size() == 2 && cfg.textures["tex0"] == "png/tex_noise2.png", "block textures");
        check(cfg.weight == 2.5 && cfg.excludeFromAuto, "block weight/auto");
    }
    {
        std::string src = "// @viz { \"name\": \"Inline\", \"weight\": 0.5 }\n#version 150\n";
        VizConfig cfg = parseVizFrontMatter(src, "shaders/x.fs");
        check(cfg.name == "Inline" && cfg.weight == 0.5, "inline form");
    }
    {
        std::string src = "// @viz-begin\n// { not json }\n// @viz-end\n#version 150\n";
        VizConfig cfg = parseVizFrontMatter(src, "shaders/broken_name.fs");
        check(cfg.name == "broken name", "invalid block falls back to defaults");
    }
    {
        std::string src = "// @viz { \"name\": \"Bad\", \"weight\": -3 }\n#version 150\n";
        VizConfig cfg = parseVizFrontMatter(src, "shaders/y.fs");
        check(cfg.weight == 1.0, "non-positive weight ignored");
    }
    if (failures == 0) {
        std::printf("ALL PASS\n");
        return 0;
    }
    std::printf("%d FAILURES\n", failures);
    return 1;
}
