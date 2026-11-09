/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

namespace v3d::render::offline::rib::requests {

/**
 * RiMakeTexture: an image made into a texture file.
 **/
struct MakeTexture {
    std::string picture;
    std::string texture;
    std::string swrap;
    std::string twrap;
    std::string filter;  /**< the name of the filter the texture is made with **/
    float swidth = 1.0F;
    float twidth = 1.0F;
};

};  // namespace v3d::render::offline::rib::requests
