// Copyright (c) 2013 Charles J. Cliffe. All rights reserved.

#include "viz/VizConfig.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {

const char *kSharedImageDir = "images";

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

std::string sidecarPath(const std::string &fragPath) {
    size_t dot = fragPath.rfind('.');
    if (dot == std::string::npos) {
        return fragPath + ".json";
    }
    return fragPath.substr(0, dot) + ".json";
}

} // namespace

VizConfig vizConfigDefaults(const std::string &fragPath) {
    VizConfig cfg;
    cfg.name = stemName(fragPath);
    return cfg;
}

VizConfig parseVizConfigJson(const std::string &jsonText, const std::string &fragPath) {
    VizConfig cfg = vizConfigDefaults(fragPath);

    try {
        json j = json::parse(jsonText);
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
    } catch (const std::exception &e) {
        printf("VizConfig: %s: ignoring invalid config (%s)\n", fragPath.c_str(), e.what());
        return vizConfigDefaults(fragPath);
    }

    return cfg;
}

VizConfig loadVizConfig(const std::string &fragPath) {
    std::ifstream in(sidecarPath(fragPath), std::ios::binary);
    if (!in) {
        return vizConfigDefaults(fragPath);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return parseVizConfigJson(ss.str(), fragPath);
}

std::string resolveVizTexturePath(const std::string &fragPath, const std::string &ref) {
    if (ref.find_first_of("/\\") == std::string::npos) {
        std::filesystem::path frag(fragPath);
        std::filesystem::path local = frag.parent_path() / ref;
        std::error_code ec;
        if (std::filesystem::exists(local, ec)) {
            return local.generic_string();
        }
        return (std::filesystem::path(kSharedImageDir) / ref).generic_string();
    }
    return ref;
}
