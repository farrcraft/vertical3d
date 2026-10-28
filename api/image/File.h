/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdio>
#include <string>

namespace v3d::image {

/**
 * Open a file for a C library that reads or writes through a FILE*, as libpng and libjpeg do.
 *
 * The path is UTF-8, as every narrow path in the api is. The caller closes the file with
 * std::fclose.
 *
 * @param mode as std::fopen takes it, such as "rb" or "wb"
 * @return the file, or null when it cannot be opened
 **/
std::FILE* openFile(const std::string& path, const char* mode);

};  // namespace v3d::image
