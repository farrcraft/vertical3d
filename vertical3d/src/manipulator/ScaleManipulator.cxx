/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ScaleManipulator.h"

#include <algorithm>

#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

namespace {

/**
 * The box at the tip of a handle, and the one at the centre, as a fraction of a
 * handle's length.
 **/
constexpr float tipSize = 0.07f;
constexpr float centreSize = 0.06f;

/**
 * The smallest a scale factor is allowed to become. A drag that ran past zero would
 * turn the object inside out and could never be dragged back, the handle having no
 * length left to grab.
 **/
constexpr float minimum = 0.01f;

};  // namespace

/**
 **/
ScaleManipulator::ScaleManipulator() {
}

/**
 **/
Manipulator::Space ScaleManipulator::effective() const noexcept {
    return Space::Local;
}

/**
 **/
void ScaleManipulator::draw(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
    v3d::render::realtime::LineCanvas* canvas) const {
    if (canvas == nullptr) {
        return;
    }
    const Placement seat = placement(mesh, view);
    if (!seat.valid) {
        return;
    }

    const Axis axes[3] = { Axis::X, Axis::Y, Axis::Z };
    for (const Axis handle : axes) {
        const glm::vec4 tint = colour(handle);
        const glm::vec3 tip = seat.origin + direction(seat, handle) * seat.size;
        canvas->line(seat.origin, tip, tint);
        marker(canvas, seat, tip, seat.size * tipSize, tint);
    }

    marker(canvas, seat, seat.origin, seat.size * centreSize, colour(Axis::None));
}

/**
 **/
void ScaleManipulator::apply(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
    const glm::vec2& from, const glm::vec2& to) const {
    if (!mesh) {
        return;
    }
    const Placement seat = placement(mesh, view);
    if (!seat.valid || seat.size <= 0.0f) {
        return;
    }

    glm::vec3 scale = mesh->scale();
    if (axis() == Axis::None) {
        // one gesture, three axes: rightwards and upwards both grow the object, which
        // is the direction a corner handle would be dragged
        const glm::vec2 delta = to - from;
        const float factor = 1.0f + (delta.x - delta.y) / handlePixels;
        scale *= factor;
    } else {
        const float factor = 1.0f + along(view, seat, axis(), from, to) / seat.size;
        switch (axis()) {
        case Axis::X:
            scale.x *= factor;
            break;
        case Axis::Y:
            scale.y *= factor;
            break;
        case Axis::Z:
            scale.z *= factor;
            break;
        case Axis::None:
        default:
            break;
        }
    }

    scale.x = std::max(scale.x, minimum);
    scale.y = std::max(scale.y, minimum);
    scale.z = std::max(scale.z, minimum);
    mesh->scale(scale);
}

};  // namespace v3d::editor
