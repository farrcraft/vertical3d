/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Vertex.h"

namespace v3d::moya {

Vertex::Vertex() {
}

Vertex::~Vertex() {
}

glm::vec3 Vertex::point(void) const {
     return point_;
}

void Vertex::point(const glm::vec3 & v) {
     point_ = v;
}

glm::vec3 Vertex::color(void) const {
     return color_;
}

void Vertex::color(const glm::vec3 & c) {
     color_ = c;
     bits_ |= HAS_COLOR;
}

};  // namespace v3d::moya
