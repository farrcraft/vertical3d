/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

// cgltf.h is a C header, and cpplint sorts it with the C system headers.
#include <cgltf.h>

#include <cstddef>

namespace v3d::asset::media::internal {

/** Where a primitive's vertices start in the model, and how many it has. **/
struct Span {
    std::size_t baseVertex = 0;
    cgltf_size count = 0;
};

};  // namespace v3d::asset::media::internal
