/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "Index.h"

#include <glm/glm.hpp>

namespace v3d::brep {

/**
 * A face of a mesh: its normal and one of the half edges around it.
 **/
class Face final {
 public:
    Face();
    Face(const glm::vec3 & normal, Index edge);
    ~Face();

    glm::vec3 normal(void) const;
    void normal(const glm::vec3 & n);
    Index edge(void) const;
    void edge(Index edgeIndex);

 private:
    glm::vec3 normal_;
    Index edge_;
};

};  // namespace v3d::brep
