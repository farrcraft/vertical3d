/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ScaleManipulator.h"

#include <api/render/realtime/canvas/Line.h>
#include <vertical3d/src/view/ViewPort.h>

#include <algorithm>

#include "Manipulator.h"
#include "Target.h"

#include <boost/shared_ptr.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

namespace {

/**
 * The box at the tip of a handle, and the one at the centre, as a fraction of a
 * handle's length.
 **/
constexpr float tipSize = 0.07F;
constexpr float centreSize = 0.06F;

/**
 * The smallest a scale factor is allowed to become. A drag that ran past zero would
 * turn the object inside out and could never be dragged back, the handle having no
 * length left to grab.
 **/

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
void ScaleManipulator::draw(const Target& target, const ViewPort& view,
    v3d::render::realtime::canvas::Line* canvas) const {
    if (canvas == nullptr) {
        return;
    }
    const Placement seat = placement(target, view);
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
bool ScaleManipulator::measure(const Placement& seat, const ViewPort& view, const glm::vec2& from,
    const glm::vec2& to, Motion* motion) const {
    if (seat.size <= 0.0F) {
        return false;
    }

    // one gesture on the centre handle scales all three axes: dragging right or up grows
    // the object, as dragging a corner handle outward would
    const glm::vec2 delta = to - from;
    const float factor = axis() == Axis::None ? 1.0F + ((delta.x - delta.y) / handlePixels) :
        1.0F + (along(view, seat, axis(), from, to) / seat.size);
    glm::vec3 stretch(factor, factor, factor);
    switch (axis()) {
    case Axis::X:
        stretch = glm::vec3(factor, 1.0F, 1.0F);
        break;
    case Axis::Y:
        stretch = glm::vec3(1.0F, factor, 1.0F);
        break;
    case Axis::Z:
        stretch = glm::vec3(1.0F, 1.0F, factor);
        break;
    case Axis::None:
    default:
        break;
    }

    // a stretch is along the handles' axes, which are each object's own
    motion->stretch = stretch;
    return true;
}

};  // namespace v3d::editor
