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

bool Vertex::hasColor(void) const {
     return (bits_ & HAS_COLOR) != 0;
}

glm::vec3 Vertex::normal(void) const {
     return normal_;
}

void Vertex::normal(const glm::vec3 & n) {
     normal_ = n;
     bits_ |= HAS_NORMAL;
}

bool Vertex::hasNormal(void) const {
     return (bits_ & HAS_NORMAL) != 0;
}

glm::vec3 Vertex::geometricNormal(void) const {
     return geometric_;
}

void Vertex::geometricNormal(const glm::vec3 & n) {
     geometric_ = n;
}

};  // namespace v3d::moya
