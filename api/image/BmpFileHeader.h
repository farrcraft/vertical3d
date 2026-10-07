/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <stdint.h>

namespace v3d::image {

#pragma pack(push, 2)

struct bmp_file_header {
    uint16_t type_;
    uint32_t size_;
    uint16_t reserved1_;
    uint16_t reserved2_;
    uint32_t offset_;
};

#pragma pack(pop)

};  // namespace v3d::image
