/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

// cgltf.h is a C header, and cpplint sorts it with the C system headers.
#include <cgltf.h>

namespace v3d::asset::media::internal {

/**
 * The accessors one primitive draws from. Only position is required. Without it there is no
 * geometry, and the other attributes default to zero.
 **/
struct Attributes final {
    const cgltf_accessor* position{ nullptr };
    const cgltf_accessor* normal{ nullptr };
    const cgltf_accessor* uv{ nullptr };
    const cgltf_accessor* joints{ nullptr };
    const cgltf_accessor* weights{ nullptr };
    bool moreInfluences{ false };  /**< a second set of joints, which four influences cannot hold **/
};

};  // namespace v3d::asset::media::internal
