/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/media/internal/Attributes.h>
// cgltf.h is a C header, and cpplint sorts it with the C system headers.
#include <cgltf.h>

#include <cstdint>

#include <glm/mat4x4.hpp>

namespace v3d::asset::media::internal {

/**
 * A primitive the traversal reached, and where its node puts it. In a model with a skeleton, a
 * skinned primitive is placed by its joints and an unskinned one follows one joint rigidly.
 **/
struct Placed final {
    const cgltf_primitive* primitive{ nullptr };
    Attributes attributes;
    glm::mat4 world{ 1.0F };
    bool skinned{ false };
    std::uint16_t joint{ 0 };  /**< the joint an unskinned primitive follows **/
};

};  // namespace v3d::asset::media::internal
