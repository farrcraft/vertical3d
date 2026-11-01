/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ConstructionPlane.h"

#include <api/render/realtime/canvas/Line.h>
#include <api/type/camera/Camera.h>

namespace v3d::editor::view {

/**
 **/
ConstructionPlane::ConstructionPlane() :
    spacing_(1.0F),
    intervals_(defaultIntervals),
    lines_(defaultLines),
    minor_(minorColour),
    major_(majorColour),
    origin_(originColour) {
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
ConstructionPlane::Axes ConstructionPlane::axes(const v3d::type::camera::Camera& camera) {
    if (camera.orthographic()) {
        // an orthographic view looks straight down an axis, so the grid lies in the plane of
        // the camera's own right and up, as in a drafting elevation
        return {.right = camera.profile().right(), .up = camera.profile().up()};
    }
    // a perspective view gets the ground plane, which is xz with y up
    return {.right = glm::vec3(1.0F, 0.0F, 0.0F), .up = glm::vec3(0.0F, 0.0F, 1.0F)};
}

/**
 **/
void ConstructionPlane::draw(const v3d::type::camera::Camera& camera, v3d::render::realtime::canvas::Line* canvas) const {
    if (canvas == nullptr || lines_ == 0 || spacing_ <= 0.0F) {
        return;
    }

    const Axes plane = axes(camera);
    const glm::vec3& right = plane.right;
    const glm::vec3& up = plane.up;

    const float extent = static_cast<float>(lines_) * spacing_ / 2.0F;

    for (unsigned int index = 0; index <= lines_; index++) {
        const float offset = -extent + (static_cast<float>(index) * spacing_);

        glm::vec4 colour = minor_;
        if (offset == 0.0F) {
            colour = origin_;
        } else if (intervals_ > 0 && (index % intervals_) == 0) {
            colour = major_;
        }

        canvas->line((right * offset) + (up * -extent), (right * offset) + (up * extent), colour);
        canvas->line((right * -extent) + (up * offset), (right * extent) + (up * offset), colour);
    }
}

};  // namespace v3d::editor::view
