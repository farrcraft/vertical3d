/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/Index.h>

#include <cstddef>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::brep::internal {

/**
 * The faces as loops of welded vertices, each clockwise seen from outside.
 **/
struct Faces {
    std::vector<glm::vec3> positions;
    std::vector<std::vector<Index>> loops;
    std::size_t corners = 0;
    std::size_t skipped = 0;
};

};  // namespace v3d::brep::internal
