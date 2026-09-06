/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RotateManipulator.h"

#include <cmath>

#include <boost/shared_ptr.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

namespace {

/**
 * How many segments a ring is drawn with.
 **/
constexpr unsigned int ringSides = 48;

/**
 * The centre handle's box, as a fraction of a ring's radius.
 **/
constexpr float centreSize = 0.06f;

/**
 * Radians a pixel, for the tumble the centre handle drives. A drag the width of a
 * handle turns the object about a third of a turn.
 **/
constexpr float tumbleRate = 0.0075f;

/**
 * How face on a ring has to be before a click can be meant for it, as the cosine of
 * the angle between its axis and the direction of view.
 *
 * A ring seen edge on projects to a line through the middle of the manipulator,
 * which crosses every other handle - and its ends land exactly on the rim of the
 * ring facing the camera, so a click meant for that rim would be taken by a ring
 * whose shape the user cannot even see. An axis view is the case that matters: two
 * of the three rings are exactly edge on there, and only the third can be turned.
 **/
constexpr float faceOn = 0.15f;

/**
 * An angle brought back into the half turn either side of zero, so a sweep across
 * the line where atan2 wraps is a small rotation rather than a whole turn.
 **/
float wrap(float angle) {
    const float turn = 2.0f * glm::pi<float>();
    while (angle > glm::pi<float>()) {
        angle -= turn;
    }
    while (angle < -glm::pi<float>()) {
        angle += turn;
    }
    return angle;
}

};  // namespace

/**
 **/
RotateManipulator::RotateManipulator() {
}

/**
 **/
float RotateManipulator::ringDistance(const Placement& placement, const glm::vec3& unit,
    const ViewPort& view, const glm::vec2& cursor) const {
    glm::vec3 first;
    glm::vec3 second;
    perpendiculars(unit, &first, &second);

    glm::vec2 start;
    bool started = false;
    float nearest = -1.0f;
    for (unsigned int step = 0; step <= ringSides; step++) {
        const float angle = 2.0f * glm::pi<float>() * static_cast<float>(step) /
            static_cast<float>(ringSides);
        const glm::vec3 point = placement.origin +
            (first * std::cos(angle) + second * std::sin(angle)) * placement.size;

        glm::vec2 end;
        if (!project(view, point, &end)) {
            started = false;
            continue;
        }
        if (!started) {
            started = true;
            start = end;
            continue;
        }

        const float distance = distanceToSegment(start, end, cursor);
        start = end;
        if (distance > tolerance) {
            continue;
        }
        if (nearest < 0.0f || distance < nearest) {
            nearest = distance;
        }
    }
    return nearest;
}

bool RotateManipulator::grab(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
    const glm::vec2& cursor, Axis* axis) const {
    const Placement seat = placement(mesh, view);
    if (!seat.valid) {
        return false;
    }

    glm::vec2 root;
    if (!project(view, seat.origin, &root)) {
        return false;
    }

    // the centre handle is the disc inside the rings rather than a point, so that a
    // tumble is as easy to start as a constrained turn
    if (glm::distance(root, cursor) <= tolerance * 1.5f) {
        if (axis != nullptr) {
            *axis = Axis::None;
        }
        return true;
    }

    // a ring is not one segment, so the base class's test does not answer for it - but
    // it is the run of segments it is drawn as, and each of those the base class can
    // answer for
    glm::vec3 forward(0.0f, 0.0f, 1.0f);  // NOLINT(build/include_what_you_use) - the direction, not std::forward
    basis(view, nullptr, nullptr, &forward);

    const Axis axes[3] = { Axis::X, Axis::Y, Axis::Z };
    bool found = false;
    float nearest = 0.0f;
    for (const Axis handle : axes) {
        const glm::vec3 unit = direction(seat, handle);
        if (std::fabs(glm::dot(unit, forward)) < faceOn) {
            continue;
        }
        const float distance = ringDistance(seat, unit, view, cursor);
        if (distance < 0.0f) {
            continue;
        }
        if (found && distance >= nearest) {
            continue;
        }
        found = true;
        nearest = distance;
        if (axis != nullptr) {
            *axis = handle;
        }
    }
    return found;
}

/**
 **/
void RotateManipulator::draw(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
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
        glm::vec3 first;
        glm::vec3 second;
        perpendiculars(direction(seat, handle), &first, &second);
        canvas->circle(seat.origin, first, second, seat.size, ringSides, colour(handle));
    }

    marker(canvas, seat, seat.origin, seat.size * centreSize, colour(Axis::None));
}

/**
 **/
float RotateManipulator::swept(const ViewPort& view, const Placement& placement, Axis axis,
    const glm::vec2& from, const glm::vec2& to) const {
    glm::vec2 root;
    if (!project(view, placement.origin, &root)) {
        return 0.0f;
    }

    const glm::vec2 before = from - root;
    const glm::vec2 after = to - root;
    // a gesture that starts on the origin has no angle about it
    if (glm::length(before) < 1.0f || glm::length(after) < 1.0f) {
        return 0.0f;
    }

    const float angle = wrap(std::atan2(after.y, after.x) - std::atan2(before.y, before.x));

    glm::vec3 forward(0.0f, 0.0f, 1.0f);  // NOLINT(build/include_what_you_use) - the direction, not std::forward
    basis(view, nullptr, nullptr, &forward);
    return glm::dot(direction(placement, axis), forward) > 0.0f ? -angle : angle;
}

/**
 **/
void RotateManipulator::apply(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
    const glm::vec2& from, const glm::vec2& to) const {
    if (!mesh) {
        return;
    }
    const Placement seat = placement(mesh, view);
    if (!seat.valid) {
        return;
    }

    glm::quat turn(1.0f, 0.0f, 0.0f, 0.0f);
    if (axis() == Axis::None) {
        glm::vec3 right(1.0f, 0.0f, 0.0f);
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        basis(view, &right, &up, nullptr);
        // negated, so the face nearest the viewer follows the cursor: a positive turn
        // about the camera's up carries the far side of the object toward the right
        const glm::vec2 delta = to - from;
        turn = glm::angleAxis(-delta.x * tumbleRate, up) * glm::angleAxis(-delta.y * tumbleRate, right);
    } else {
        const glm::vec3 unit = direction(seat, axis());
        const float angle = swept(view, seat, axis(), from, to);
        if (angle == 0.0f) {
            return;
        }
        turn = glm::angleAxis(angle, glm::normalize(unit));
    }

    // on the left: the turn is measured in world space and the object's own rotation is
    // what it turns
    mesh->rotation(glm::normalize(turn * mesh->rotation()));
}

};  // namespace v3d::editor
