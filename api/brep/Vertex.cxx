/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Vertex.h"

#include "Index.h"


namespace v3d::brep {

Vertex::Vertex() : edge_(static_cast<Index>(INVALID_ID)), selected_(false) {
}

Vertex::Vertex(const glm::vec3& position) : point_(position), edge_(static_cast<Index>(INVALID_ID)), selected_(false) {
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

Index Vertex::edge(void) const {
    return edge_;
}

void Vertex::edge(Index edgeIndex) {
    edge_ = edgeIndex;
}

glm::vec3 Vertex::point(void) const {
    return point_;
}

void Vertex::point(const glm::vec3& position) {
    point_ = position;
}

};  // namespace v3d::brep
