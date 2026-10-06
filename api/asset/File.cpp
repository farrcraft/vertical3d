/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "File.h"

#include <cstddef>
#include <fstream>
#include <string>

namespace v3d::asset {

std::optional<std::string> readFile(std::string_view path) {
    // opened at the end, so the position is the size of the file
    std::ifstream file(std::string(path), std::ios::in | std::ios::binary | std::ios::ate);
    if (!file) {
        return std::nullopt;
    }
    const std::streamoff size = file.tellg();
    if (size < 0) {
        return std::nullopt;
    }
    file.seekg(0, std::ios::beg);
    if (!file) {
        return std::nullopt;
    }
    std::string content(static_cast<std::size_t>(size), '\0');
    // a read that returns fewer bytes than the file holds sets failbit, and is a failure
    if (size > 0 && (!file.read(content.data(), static_cast<std::streamsize>(size)) || file.gcount() != size)) {
        return std::nullopt;
    }
    return content;
}

};  // namespace v3d::asset
