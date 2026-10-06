/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SearchPath.h"

#include <cctype>
#include <string>
#include <vector>

namespace v3d::render::offline {

namespace {

std::vector<std::string> split(const std::string & path) {
    std::vector<std::string> found;
    std::string current;
    for (std::size_t i = 0; i < path.size(); i++) {
        const bool drive = path[i] == ':' && current.size() == 1 && std::isalpha(static_cast<unsigned char>(current[0])) != 0;
        if (path[i] != ':' || drive) {
            current += path[i];
            continue;
        }
        if (!current.empty()) {
            found.push_back(current);
        }
        current.clear();
    }
    if (!current.empty()) {
        found.push_back(current);
    }
    return found;
}

};  // namespace

std::vector<std::string> searchpath(const std::string & path, const std::vector<std::string> & before) {
    std::vector<std::string> next;
    for (const std::string & directory : split(path)) {
        if (directory != "&") {
            next.push_back(directory);
            continue;
        }
        for (const std::string & held : before) {
            next.push_back(held);
        }
    }
    return next;
}

};  // namespace v3d::render::offline
