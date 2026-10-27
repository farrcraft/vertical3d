/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Face.h"

#include "Index.h"

namespace v3d::brep {

Face::Face() : normal_(0.0F, 0.0F, 0.0F), edge_(INVALID_ID) {
}

Face::Face(const glm::vec3& normal, Index edge) : normal_(normal), edge_(edge) {
}

Face::~Face() {
}

glm::vec3 Face::normal(void) const {
    return normal_;
}

void Face::normal(const glm::vec3& n) {
    normal_ = n;
}

Index Face::edge(void) const {
    return edge_;
}

void Face::edge(Index edgeIndex) {
    edge_ = edgeIndex;
}

};  // namespace v3d::brep
