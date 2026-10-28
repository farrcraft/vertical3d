/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "File.h"

#include <cstdio>
#include <string>

namespace v3d::image {

std::FILE* openFile(const std::string& path, const char* mode) {
#ifdef _MSC_VER
    // MSVC deprecates std::fopen in favour of its own checked form, and the build treats the
    // warning as an error. Other compilers have no fopen_s
    std::FILE* file = nullptr;
    return fopen_s(&file, path.c_str(), mode) == 0 ? file : nullptr;
#else
    return std::fopen(path.c_str(), mode);
#endif
}

};  // namespace v3d::image
