/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ConstructionPlane.h"

namespace v3d::editor {

/**
 **/
ConstructionPlane::ConstructionPlane() :
    spacing_(1.0f),
    intervals_(10),
    lines_(60),
    minor_(0.30f, 0.30f, 0.30f, 1.0f),
    major_(0.42f, 0.42f, 0.52f, 1.0f),
    origin_(0.65f, 0.65f, 0.70f, 1.0f) {
}

/**
 **/
void ConstructionPlane::spacing(float spacing) noexcept {
    spacing_ = spacing;
}

/**
 **/
float ConstructionPlane::spacing() const noexcept {
    return spacing_;
}

/**
 **/
void ConstructionPlane::intervals(unsigned int intervals) noexcept {
    intervals_ = intervals;
}

/**
 **/
unsigned int ConstructionPlane::intervals() const noexcept {
    return intervals_;
}

/**
 **/
void ConstructionPlane::lines(unsigned int lines) noexcept {
    lines_ = lines;
}

/**
 **/
unsigned int ConstructionPlane::lines() const noexcept {
    return lines_;
}

/**
 **/
void ConstructionPlane::axes(const v3d::type::Camera& camera, glm::vec3* right, glm::vec3* up) const {
    if (camera.orthographic()) {
        // an orthographic view looks straight down an axis, so the grid in the plane of
        // the camera's own right and up is the one that reads as a drafting elevation
        *right = camera.profile().right();
        *up = camera.profile().up();
        return;
    }
    // a perspective view gets the ground plane, which is xz with y up
    *right = glm::vec3(1.0f, 0.0f, 0.0f);
    *up = glm::vec3(0.0f, 0.0f, 1.0f);
}

/**
 **/
void ConstructionPlane::draw(const v3d::type::Camera& camera, v3d::render::realtime::LineCanvas* canvas) const {
    if (canvas == nullptr || lines_ == 0 || spacing_ <= 0.0f) {
        return;
    }

    glm::vec3 right(1.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 0.0f, 1.0f);
    axes(camera, &right, &up);

    const float extent = static_cast<float>(lines_) * spacing_ / 2.0f;

    for (unsigned int index = 0; index <= lines_; index++) {
        const float offset = -extent + (static_cast<float>(index) * spacing_);

        glm::vec4 colour = minor_;
        if (offset == 0.0f) {
            colour = origin_;
        } else if (intervals_ > 0 && (index % intervals_) == 0) {
            colour = major_;
        }

        canvas->line((right * offset) + (up * -extent), (right * offset) + (up * extent), colour);
        canvas->line((right * -extent) + (up * offset), (right * extent) + (up * offset), colour);
    }
}

};  // namespace v3d::editor
