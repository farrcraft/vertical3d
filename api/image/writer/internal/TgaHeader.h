/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

namespace v3d::image::writer::internal {

#pragma pack(push, 1)

struct tga_header {
    unsigned char id_;
    unsigned char colormap_;  // 0=none, 1=palette
    unsigned char type_;  // 0=none, 1=indexed, 2=rgb, 3=grey, 4=rle

    uint16_t cmap_start_;
    uint16_t cmap_length_;
    unsigned char cmap_bits_;  // 15, 16, 24, 32

    uint16_t xorigin_;
    uint16_t yorigin_;
    uint16_t width_;
    uint16_t height_;
    unsigned char bpp_;  // 8, 16, 24, 32
    unsigned char descriptor_;
};

#pragma pack(pop)

};  // namespace v3d::image::writer::internal
