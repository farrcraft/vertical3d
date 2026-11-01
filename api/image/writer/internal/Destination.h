/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/format/JpegError.h>

#include <array>
#include <string>

namespace v3d::image::writer::internal {

/**
 * Where libjpeg puts what it encodes. It fills the chunk, and each full chunk, and the part
 * of the last one it filled, go on the end of the string.
 **/
struct Destination {
    jpeg_destination_mgr manager;  // first, so the library's pointer to it is one of these
    std::array<JOCTET, 4096> chunk;
    std::string* bytes;
};

};  // namespace v3d::image::writer::internal
