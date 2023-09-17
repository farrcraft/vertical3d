/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

namespace v3d::asset {
    /**
     **/
    enum class Type {
        Undefined = 0,
        ImagePng = 1,
        ImageJpeg = 2,
        JsonDocument = 3,
        AudioWav = 4,
        Text = 5,
        ShaderProgram = 6,
        ShaderVertex = 7,
        ShaderFragment = 8,
        Font2D = 9,
        TextureFont = 10
    };
};  // namespace v3d::asset
