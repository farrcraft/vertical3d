/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Words.h"

#include <sstream>
#include <string>
#include <vector>

namespace v3d::render::offline::rib {

std::vector<std::string> words(const std::string & text) {
    std::istringstream stream(text);
    std::vector<std::string> result;
    std::string word;
    while (stream >> word) {
        result.push_back(word);
    }
    return result;
}

};  // namespace v3d::render::offline::rib
