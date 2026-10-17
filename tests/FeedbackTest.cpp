#include <cstdio>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>

#include "viz/VizConfig.h"

// Feedback-shader contract: every roster visual sampling backbuffer must be
// tagged, fade with the transition alpha, and need no inputs the renderer
// never provides.

static int failures = 0;

static void check(bool ok, const char *name) {
    if (!ok) {
        ++failures;
        std::printf("FAIL %s\n", name);
    } else {
        std::printf("ok %s\n", name);
    }
}

static std::string readAll(const std::filesystem::path &p) {
    std::ifstream in(p, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main() {
    std::regex backbufferDecl("uniform\\s+sampler2D\\s+backbuffer\\s*;");
    int feedbackCount = 0;

    std::error_code ec;
    std::filesystem::recursive_directory_iterator it("shaders/visuals", ec);
    if (ec) {
        std::printf("FAIL cannot scan shaders/visuals (%s)\n", ec.message().c_str());
        return 1;
    }
    for (auto &entry : it) {
        if (!entry.is_regular_file() || entry.path().extension() != ".fs") {
            continue;
        }
        std::string frag = entry.path().generic_string();
        std::string src = readAll(entry.path());
        if (!std::regex_search(src, backbufferDecl)) {
            continue;
        }
        ++feedbackCount;

        VizConfig cfg = loadVizConfig(frag);
        bool tagged = false;
        for (auto &t : cfg.tags) {
            if (t == "feedback") {
                tagged = true;
            }
        }
        check(tagged, (frag + " sidecar tags feedback").c_str());
        check(src.find("blendAlpha") != std::string::npos,
            (frag + " fades with blendAlpha").c_str());
        check(src.find("surfaceSize") == std::string::npos &&
                src.find("surfacePosition") == std::string::npos,
            (frag + " needs no surface inputs").c_str());
    }
    check(feedbackCount > 0, "at least one feedback visualizer");
    if (failures == 0) {
        std::printf("ALL PASS\n");
        return 0;
    }
    std::printf("%d FAILURES\n", failures);
    return 1;
}
