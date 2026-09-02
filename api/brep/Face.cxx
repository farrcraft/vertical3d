/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Face.h"

namespace v3d::brep {

    Face::Face() : selected_(false) {
    }

    Face::Face(const glm::vec3& normal, unsigned int edge) : normal_(normal), edge_(edge), selected_(false) {
    }

    Face::~Face() {
    }

    bool Face::selected(void) const noexcept {
        return selected_;
    }

    void Face::selected(bool sel) noexcept {
        selected_ = sel;
    }

    glm::vec3 Face::normal(void) const {
        return normal_;
    }

    void Face::normal(const glm::vec3& n) {
        normal_ = n;
    }

    unsigned int Face::edge(void) const {
        return edge_;
    }

    void Face::edge(unsigned int e) {
        edge_ = e;
    }

};  // namespace v3d::brep
