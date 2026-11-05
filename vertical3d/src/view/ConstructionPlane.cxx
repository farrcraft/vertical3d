/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ConstructionPlane.h"

#include <api/render/realtime/canvas/Line.h>
#include <api/type/camera/Camera.h>

#include <cstddef>

#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::view {

namespace {

/**
 * @return the world axis a direction runs most nearly along: 0, 1 or 2 for x, y and z
 **/
std::size_t dominant(const glm::vec3& direction) {
    const glm::vec3 size = glm::abs(direction);
    if (size.x >= size.y && size.x >= size.z) {
        return 0;
    }
    return size.y >= size.z ? 1 : 2;
}

};  // namespace

/**
 **/
ConstructionPlane::ConstructionPlane() :
    spacing_(1.0F),
    intervals_(defaultIntervals),
    lines_(defaultLines),
    colours_{ .minor = minorColour, .major = majorColour, .axes = { originColour, originColour, originColour } } {
}

/**
 **/
void ConstructionPlane::colours(const Colours& colours) noexcept {
    colours_ = colours;
}

/**
 **/
const ConstructionPlane::Colours& ConstructionPlane::colours() const noexcept {
    return colours_;
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
    // a line through the origin takes the colour of the world axis it runs along
    const glm::vec4& alongUp = colours_.axes[dominant(up)];
    const glm::vec4& alongRight = colours_.axes[dominant(right)];

    for (unsigned int index = 0; index <= lines_; index++) {
        const float offset = -extent + (static_cast<float>(index) * spacing_);

        const bool origin = offset == 0.0F;
        const glm::vec4& colour = intervals_ > 0 && (index % intervals_) == 0 ? colours_.major : colours_.minor;
        canvas->line((right * offset) + (up * -extent), (right * offset) + (up * extent), origin ? alongUp : colour);
        canvas->line((right * -extent) + (up * offset), (right * extent) + (up * offset), origin ? alongRight : colour);
    }
}

};  // namespace v3d::editor::view
