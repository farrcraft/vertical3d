/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::brep {

/**
 * Faces as a file holds them: a list of points, and each face as a run of indices into it.
 *
 * counts holds how many corners each face has, in order, and indices holds every face's
 * corners one face after another. A point may appear several times at the same position, as it
 * does where a file splits a vertex for its normals or texture coordinates.
 **/
struct Polygons {
    std::vector<glm::vec3> points;
    std::vector<std::uint32_t> counts;
    std::vector<std::uint32_t> indices;
};

};  // namespace v3d::brep
