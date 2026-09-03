/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Vertex.h"

#include "HalfEdge.h"

namespace v3d::brep {

Vertex::Vertex() : edge_(static_cast<unsigned int>(INVALID_ID)), selected_(false) {
}

Vertex::Vertex(const glm::vec3& p) : point_(p), edge_(static_cast<unsigned int>(INVALID_ID)), selected_(false) {
}

Vertex::~Vertex() {
}

bool Vertex::operator == (const Vertex & v) const {
    return (point_ == v.point_);
}

bool Vertex::operator == (const glm::vec3 & v) const {
    return (point_ == v);
}

bool Vertex::selected(void) const noexcept {
    return selected_;
}

void Vertex::selected(bool sel) noexcept {
    selected_ = sel;
}

unsigned int Vertex::edge(void) const {
    return edge_;
}

void Vertex::edge(unsigned int e) {
    edge_ = e;
}

glm::vec3 Vertex::point(void) const {
    return point_;
}

void Vertex::point(const glm::vec3& p) {
    point_ = p;
}

};  // namespace v3d::brep
