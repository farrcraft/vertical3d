/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "File.h"

#include <fstream>
#include <iterator>
#include <string>

namespace v3d::asset {

std::optional<std::string> readFile(std::string_view path) {
    std::ifstream file(std::string(path), std::ios::in | std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (file.bad()) {
        return std::nullopt;
    }
    return content;
}

};  // namespace v3d::asset
