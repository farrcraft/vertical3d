/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <stdint.h>

namespace v3d::image {

#pragma pack(push, 2)

struct bmp_info_header {
    uint32_t size_;
    int32_t width_;
    int32_t height_;
    uint16_t planes_;
    uint16_t bits_;
    uint32_t compression_;
    uint32_t imageSize_;
    int32_t xppm_;
    int32_t yppm_;
    uint32_t used_;
    uint32_t important_;
};

#pragma pack(pop)

};  // namespace v3d::image
