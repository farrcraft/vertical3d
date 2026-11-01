/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

// cgltf.h is a C header, and cpplint sorts it with the C system headers.
#include <cgltf.h>

#include <cstdint>
#include <map>
#include <vector>

#include <glm/mat4x4.hpp>

namespace v3d::asset::media::internal {

/**
 * The skin a model keeps, and how to reach its joints in the skeleton's order.
 **/
struct Rig final {
    const cgltf_skin* skin{ nullptr };
    std::vector<std::uint16_t> order;                     /**< a skin joint's index to the skeleton's **/
    std::map<const cgltf_node*, std::uint16_t> joints;    /**< a joint's node to the skeleton's index **/
    std::vector<glm::mat4> bound;                         /**< by skeleton index: the joint at rest, times its inverse bind **/
};

};  // namespace v3d::asset::media::internal
