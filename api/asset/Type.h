/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

namespace v3d::asset {
    /**
     **/
    enum class Type {
        TYPE_UNDEFINED = 0,
        IMAGE_PNG = 1,
        IMAGE_JPEG = 2,
        JSON_DOCUMENT = 3,
        AUDIO_WAV = 4,
        TEXT = 5,
        SHADER_PROGRAM = 6,
        SHADER_VERTEX = 7,
        SHADER_FRAGMENT = 8,
        FONT_2D = 9
    };
};  // namespace v3d::asset
