/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/texture/Image.h>
#include <moya/libmoya/kernel/texture/Texture.h>

#include <cstdint>

namespace v3d::moya::kernel::texture {

/**
 * The images a scene's programs read: each image's grid, the texture of each place in every
 * grid, and each texture's size. An image node names its image by its place here.
 **/
struct Catalog {
    Array<Image> images;
    Array<std::uint32_t> tiles;
    Array<Texture> textures;
};

};  // namespace v3d::moya::kernel::texture
