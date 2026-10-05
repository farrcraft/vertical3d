/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::image {

#pragma pack(push, 2)

struct bmp_rgb_quad {
    unsigned char blue_;
    unsigned char green_;
    unsigned char red_;
    unsigned char reserved_;
};

#pragma pack(pop)

};  // namespace v3d::image
