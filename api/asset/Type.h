/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <cstdint>

namespace v3d::asset {
/**
 **/
enum class Type : std::uint8_t {
    Undefined = 0,
    ImagePng = 1,
    ImageJpeg = 2,
    JsonDocument = 3,
    AudioWav = 4,
    Text = 5,
    ImageTga = 11,
    ModelGltf = 12,
    ImageBmp = 13
};
};  // namespace v3d::asset
