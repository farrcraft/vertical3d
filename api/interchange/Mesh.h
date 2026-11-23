/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::interchange {

/**
 * A polygon mesh in its node's space: a list of points, and each face as a run of indices into
 * it. counts holds how many corners each face has, and indices holds every face's corners one
 * face after another. A face runs counter-clockwise seen from outside.
 **/
struct Mesh {
    std::string name;
    std::vector<glm::vec3> points;
    std::vector<std::uint32_t> counts;
    std::vector<std::uint32_t> indices;
    /**
     * Whether the format stores triangles only, so that a quad the author made arrives cut in
     * two.
     **/
    bool triangulated = false;
};

};  // namespace v3d::interchange
