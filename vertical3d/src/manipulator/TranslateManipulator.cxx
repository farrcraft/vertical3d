/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TranslateManipulator.h"

#include <cmath>

#include <boost/shared_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

namespace {

/**
 * The arrow head at the tip of a handle, as a fraction of the handle's length.
 **/
constexpr float headLength = 0.22F;
constexpr float headRadius = 0.07F;

/**
 * How many segments the head's ring is drawn with, and how many of them are joined
 * back to the tip.
 **/
constexpr unsigned int headSides = 12;
constexpr unsigned int headSpokes = 4;

/**
 * The centre handle's box, as a fraction of a handle's length.
 **/
constexpr float centreSize = 0.06F;

};  // namespace

/**
 **/
TranslateManipulator::TranslateManipulator() {
}

/**
 **/
void TranslateManipulator::draw(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
    v3d::render::realtime::canvas::Line* canvas) const {
    if (canvas == nullptr) {
        return;
    }
    const Placement seat = placement(mesh, view);
    if (!seat.valid) {
        return;
    }

    const Axis axes[3] = { Axis::X, Axis::Y, Axis::Z };
    for (const Axis handle : axes) {
        const glm::vec3 unit = direction(seat, handle);
        const glm::vec3 tip = seat.origin + unit * seat.size;
        const glm::vec4 tint = colour(handle);

        canvas->line(seat.origin, tip, tint);

        // the head is a cone, and a cone drawn in lines is its base ring and a few of
        // its edges
        const glm::vec3 base = tip - unit * (seat.size * headLength);
        const float radius = seat.size * headRadius;
        glm::vec3 first;
        glm::vec3 second;
        perpendiculars(unit, &first, &second);
        canvas->circle({.centre = base, .axisU = first, .axisV = second, .radius = radius, .sides = headSides}, tint);
        for (unsigned int spoke = 0; spoke < headSpokes; spoke++) {
            const float angle = 2.0F * glm::pi<float>() * static_cast<float>(spoke) /
                static_cast<float>(headSpokes);
            const glm::vec3 rim = base + (first * std::cos(angle) + second * std::sin(angle)) * radius;
            canvas->line(rim, tip, tint);
        }
    }

    marker(canvas, seat, seat.origin, seat.size * centreSize, colour(Axis::None));
}

/**
 **/
void TranslateManipulator::apply(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
    const glm::vec2& from, const glm::vec2& to) const {
    if (!mesh) {
        return;
    }
    const Placement seat = placement(mesh, view);
    if (!seat.valid) {
        return;
    }

    glm::vec3 offset(0.0F, 0.0F, 0.0F);
    if (axis() == Axis::None) {
        // the centre handle: both components of the gesture move the object, in the
        // plane of the screen. Screen y points down, so a downward drag runs against
        // the camera's up
        const Basis axes = basis(view);
        const glm::vec3& right = axes.right;
        const glm::vec3& up = axes.up;
        const float scale = unitsPerPixel(view, seat.origin);
        const glm::vec2 delta = to - from;
        offset = right * (delta.x * scale) - up * (delta.y * scale);
    } else {
        offset = direction(seat, axis()) * along(view, seat, axis(), from, to);
    }

    // by an offset, not to a position: a drag measures how far the gesture went
    mesh->translate(offset);
}

};  // namespace v3d::editor
