// Copyright (c) 2013 Charles J. Cliffe. All rights reserved.

#include "viz/VizConfig.h"

#include <cmath>
#include <cstdio>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {

std::string stemName(const std::string &fragPath) {
    size_t slash = fragPath.find_last_of("/\\");
    std::string base = (slash == std::string::npos) ? fragPath : fragPath.substr(slash + 1);
    size_t dot = base.rfind('.');
    if (dot != std::string::npos) {
        base = base.substr(0, dot);
    }
    for (char &c : base) {
        if (c == '_' || c == '-') {
            c = ' ';
        }
    }
    return base.empty() ? fragPath : base;
}

// Strip one leading "//" comment marker (plus surrounding whitespace) so the
// JSON can live inside a comment block without breaking GLSL compilation.
std::string uncommentLine(const std::string &line) {
    size_t i = line.find_first_not_of(" \t\r");
    std::string s = (i == std::string::npos) ? std::string() : line.substr(i);
    if (s.size() >= 2 && s[0] == '/' && s[1] == '/') {
        s = s.substr(2);
        i = s.find_first_not_of(" \t");
        s = (i == std::string::npos) ? std::string() : s.substr(i);
    }
    return s;
}

bool extractBlock(const std::string &fileText, std::string &outJson) {
    size_t begin = fileText.find("@viz-begin");
    if (begin == std::string::npos) {
        return false;
    }
    size_t end = fileText.find("@viz-end", begin);
    if (end == std::string::npos || end <= begin) {
        return false;
    }
    size_t lineStart = fileText.find('\n', begin);
    if (lineStart == std::string::npos || lineStart > end) {
        return false;
    }
    std::string body;
    size_t pos = lineStart + 1;
    while (pos < end) {
        size_t nl = fileText.find('\n', pos);
        size_t stop = (nl == std::string::npos || nl > end) ? end : nl;
        body += uncommentLine(fileText.substr(pos, stop - pos));
        body += '\n';
        if (nl == std::string::npos || nl >= end) {
            break;
        }
        pos = nl + 1;
    }
    outJson = body;
    return true;
}

// Single-line form: // @viz { ... } — brace match honors strings/escapes.
bool extractInline(const std::string &fileText, std::string &outJson) {
    size_t mark = fileText.find("@viz");
    if (mark == std::string::npos) {
        return false;
    }
    size_t open = fileText.find('{', mark);
    size_t lineEnd = fileText.find('\n', mark);
    if (open == std::string::npos || (lineEnd != std::string::npos && open > lineEnd)) {
        return false;
    }
    bool inStr = false;
    bool esc = false;
    int depth = 0;
    for (size_t i = open; i < fileText.size(); i++) {
        char c = fileText[i];
        if (c == '\n') {
            break;
        }
        if (inStr) {
            if (esc) {
                esc = false;
            } else if (c == '\\') {
                esc = true;
            } else if (c == '"') {
                inStr = false;
            }
            continue;
        }
        if (c == '"') {
            inStr = true;
        } else if (c == '{') {
            depth++;
        } else if (c == '}') {
            depth--;
            if (depth == 0) {
                outJson = fileText.substr(open, i - open + 1);
                return true;
            }
        }
    }
    return false;
}

} // namespace

VizConfig vizConfigDefaults(const std::string &fragPath) {
    VizConfig cfg;
    cfg.name = stemName(fragPath);
    return cfg;
}

VizConfig parseVizFrontMatter(const std::string &fileText, const std::string &fragPath) {
    VizConfig cfg = vizConfigDefaults(fragPath);

    std::string raw;
    if (!extractBlock(fileText, raw) && !extractInline(fileText, raw)) {
        return cfg;
    }

    try {
        json j = json::parse(raw);
        if (j.contains("version") && j["version"].is_number_integer()) {
            cfg.version = j["version"].get<int>();
        }
        if (j.contains("name") && j["name"].is_string()) {
            std::string name = j["name"].get<std::string>();
            if (!name.empty()) {
                cfg.name = name;
            }
        }
        if (j.contains("description") && j["description"].is_string()) {
            cfg.description = j["description"].get<std::string>();
        }
        if (j.contains("author") && j["author"].is_string()) {
            cfg.author = j["author"].get<std::string>();
        }
        if (j.contains("tags") && j["tags"].is_array()) {
            for (auto &t : j["tags"]) {
                if (t.is_string()) {
                    cfg.tags.push_back(t.get<std::string>());
                }
            }
        }
        if (j.contains("textures") && j["textures"].is_object()) {
            for (auto it = j["textures"].begin(); it != j["textures"].end(); ++it) {
                if (it.value().is_string()) {
                    std::string path = it.value().get<std::string>();
                    if (!it.key().empty() && !path.empty()) {
                        cfg.textures[it.key()] = path;
                    }
                }
            }
        }
        if (j.contains("weight") && j["weight"].is_number()) {
            double w = j["weight"].get<double>();
            if (std::isfinite(w) && w > 0.0) {
                cfg.weight = w;
            }
        }
        if (j.contains("excludeFromAuto") && j["excludeFromAuto"].is_boolean()) {
            cfg.excludeFromAuto = j["excludeFromAuto"].get<bool>();
        }
    } catch (const std::exception &e) {
        printf("VizConfig: %s: ignoring invalid front-matter (%s)\n", fragPath.c_str(), e.what());
        return vizConfigDefaults(fragPath);
    }

    return cfg;
}
