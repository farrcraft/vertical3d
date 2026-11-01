/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/media/internal/Placed.h>
// cgltf.h is a C header, and cpplint sorts it with the C system headers.
#include <cgltf.h>

#include <vector>

namespace v3d::asset::media::internal {

/**
 * The primitives drawn with one material, which become one part. The material is null for
 * primitives that name none.
 **/
struct Bucket final {
    const cgltf_material* material{ nullptr };
    std::vector<Placed> primitives;
};

};  // namespace v3d::asset::media::internal
