/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Overlay.h"

namespace v3d::ui {

    Overlay::Overlay() : mode_(MODE_TRANSPARENT) {
    }

    Overlay::Overlay(Mode m) : mode_(m) {
    }

    Overlay::Overlay(const glm::vec3& c) : color_(c), mode_(MODE_COLOR) {
    }

    void Overlay::color(const glm::vec3& c) {
        color_ = c;
    }

    glm::vec3 Overlay::color() const {
        return color_;
    }

    Overlay::Mode Overlay::mode() const {
        return mode_;
    }

};  // end namespace v3d::ui
