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
        VizConfig cfg = loadVizConfig("shaders/does_not_exist.fs");
        check(cfg.name == "does not exist", "defaults derive name from file stem");
        check(cfg.textures.empty(), "defaults have no textures");
        check(cfg.weight == 1.0, "default weight");
    }
    {
        std::string doc =
            "{\n"
            "  \"name\": \"Voxel Edges\",\n"
            "  \"tags\": [\"voxel\", \"raymarch\"],\n"
            "  \"textures\": { \"tex0\": \"tex_noise2.png\", \"tex1\": \"tex_tex07.png\" },\n"
            "  \"weight\": 2.5,\n"
            "  \"futureField\": [1, 2, 3]\n"
            "}\n";
        VizConfig cfg = parseVizConfigJson(doc, "shaders/voxel_edges.fs");
        check(cfg.name == "Voxel Edges", "sidecar name");
        check(cfg.tags.size() == 2 && cfg.tags[0] == "voxel", "sidecar tags");
        check(cfg.textures.size() == 2 && cfg.textures["tex0"] == "tex_noise2.png", "sidecar textures");
        check(cfg.weight == 2.5, "sidecar weight");
    }
    {
        VizConfig cfg = parseVizConfigJson("{ not json }", "shaders/broken_name.fs");
        check(cfg.name == "broken name", "invalid sidecar falls back to defaults");
    }
    {
        VizConfig cfg = parseVizConfigJson("{ \"name\": \"Bad\", \"weight\": -3 }", "shaders/y.fs");
        check(cfg.weight == 1.0, "non-positive weight ignored");
    }
    {
        check(resolveVizTexturePath("shaders/visuals/demo/x.fs", "images/tex.png") == "images/tex.png",
            "explicit path used verbatim");
        check(resolveVizTexturePath("shaders/visuals/demo/x.fs", "png/tex.png") == "png/tex.png",
            "legacy path used verbatim");
        check(resolveVizTexturePath("shaders/visuals/demo/x.fs", "missing.png") == "images/missing.png",
            "bare name falls back to shared image folder");
    }
    if (failures == 0) {
        std::printf("ALL PASS\n");
        return 0;
    }
    std::printf("%d FAILURES\n", failures);
    return 1;
}
