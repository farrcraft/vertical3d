/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Manipulator.h"

#include <cmath>

#include <boost/shared_ptr.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

namespace {

/**
 * The three axis colours, and the colour a handle takes while it is the active one.
 **/
constexpr glm::vec4 xColour(0.90f, 0.28f, 0.32f, 1.0f);
constexpr glm::vec4 yColour(0.42f, 0.82f, 0.35f, 1.0f);
constexpr glm::vec4 zColour(0.32f, 0.55f, 0.95f, 1.0f);
constexpr glm::vec4 centreColour(0.80f, 0.82f, 0.86f, 1.0f);
constexpr glm::vec4 activeColour(1.0f, 0.86f, 0.25f, 1.0f);

};  // namespace

const float Manipulator::handlePixels = 90.0f;
const float Manipulator::tolerance = 6.0f;

/**
 **/
Manipulator::Placement::Placement() :
valid(false),
origin(0.0f, 0.0f, 0.0f),
orientation(1.0f, 0.0f, 0.0f, 0.0f),
size(0.0f) {
}

/**
 **/
Manipulator::Manipulator() :
    axis_(Axis::None),
    active_(false),
    space_(Space::Global) {
}

/**
 **/
Manipulator::~Manipulator() {
}

/**
 **/
Manipulator::Axis Manipulator::axis() const noexcept {
    return axis_;
}

/**
 **/
void Manipulator::axis(Axis axis) noexcept {
    axis_ = axis;
}

/**
 **/
bool Manipulator::active() const noexcept {
    return active_;
}

/**
 **/
void Manipulator::active(bool active) noexcept {
    active_ = active;
}

/**
 **/
Manipulator::Space Manipulator::space() const noexcept {
    return space_;
}

/**
 **/
void Manipulator::space(Space space) noexcept {
    space_ = space;
}

/**
 **/
Manipulator::Space Manipulator::effective() const noexcept {
    return space_;
}

/**
 **/
Manipulator::Placement Manipulator::placement(const boost::shared_ptr<v3d::brep::BRep>& mesh,
    const ViewPort& view) const {
    Placement placement;
    if (!mesh) {
        return placement;
    }

    boost::shared_ptr<v3d::type::camera::Camera> camera = view.camera();
    const glm::vec4& region = view.region();
    if (!camera || region.z <= 0.0f || region.w <= 0.0f) {
        return placement;
    }

    // the camera as it stands, not as it was last drawn: a drag arrives between frames
    camera->createProjection();
    camera->createView();

    placement.origin = mesh->translation();
    placement.orientation = effective() == Space::Local ? mesh->rotation() : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    placement.size = handlePixels * unitsPerPixel(view, placement.origin);
    placement.valid = placement.size > 0.0f;
    return placement;
}

/**
 **/
glm::vec3 Manipulator::direction(const Placement& placement, Axis axis) {
    glm::vec3 unit(0.0f, 0.0f, 0.0f);
    switch (axis) {
    case Axis::X:
        unit.x = 1.0f;
        break;
    case Axis::Y:
        unit.y = 1.0f;
        break;
    case Axis::Z:
        unit.z = 1.0f;
        break;
    case Axis::None:
    default:
        return unit;
    }
    return placement.orientation * unit;
}

/**
 **/
glm::vec4 Manipulator::colour(Axis axis) const {
    if (active_ && axis == axis_) {
        return activeColour;
    }
    switch (axis) {
    case Axis::X:
        return xColour;
    case Axis::Y:
        return yColour;
    case Axis::Z:
        return zColour;
    case Axis::None:
    default:
        return centreColour;
    }
}

/**
 **/
bool Manipulator::project(const ViewPort& view, const glm::vec3& point, glm::vec2* position) {
    boost::shared_ptr<v3d::type::camera::Camera> camera = view.camera();
    const glm::vec4& region = view.region();
    if (!camera || region.z <= 0.0f || region.w <= 0.0f) {
        return false;
    }

    int viewport[4];
    viewport[0] = static_cast<int>(region.x);
    viewport[1] = static_cast<int>(region.y);
    viewport[2] = static_cast<int>(region.z);
    viewport[3] = static_cast<int>(region.w);

    const glm::vec3 projected = camera->project(point, viewport);
    // outside the depth range is behind the near plane or beyond the far one, and a
    // point behind a perspective camera lands beyond it rather than looking like one in
    // front
    if (projected.z < 0.0f || projected.z > 1.0f) {
        return false;
    }
    if (position != nullptr) {
        *position = glm::vec2(projected.x, projected.y);
    }
    return true;
}

/**
 **/
float Manipulator::unitsPerPixel(const ViewPort& view, const glm::vec3& at) {
    boost::shared_ptr<v3d::type::camera::Camera> camera = view.camera();
    const glm::vec4& region = view.region();
    if (!camera || region.z <= 0.0f || region.w <= 0.0f) {
        return 0.0f;
    }

    int viewport[4];
    viewport[0] = static_cast<int>(region.x);
    viewport[1] = static_cast<int>(region.y);
    viewport[2] = static_cast<int>(region.z);
    viewport[3] = static_cast<int>(region.w);

    const glm::vec3 projected = camera->project(at, viewport);
    const glm::vec3 here = camera->unproject(projected, viewport);
    const glm::vec3 next = camera->unproject(glm::vec3(projected.x + 1.0f, projected.y, projected.z), viewport);
    return glm::length(next - here);
}

/**
 **/
void Manipulator::basis(const ViewPort& view, glm::vec3* right, glm::vec3* up, glm::vec3* forward) {
    boost::shared_ptr<v3d::type::camera::Camera> camera = view.camera();
    if (!camera) {
        return;
    }
    // the view matrix rotates the world into the camera's axes, so its rows are those
    // axes in world space
    const glm::mat4 matrix = camera->view();
    if (right != nullptr) {
        *right = glm::vec3(matrix[0][0], matrix[1][0], matrix[2][0]);
    }
    if (up != nullptr) {
        *up = glm::vec3(matrix[0][1], matrix[1][1], matrix[2][1]);
    }
    if (forward != nullptr) {
        *forward = glm::vec3(matrix[0][2], matrix[1][2], matrix[2][2]);
    }
}

/**
 **/
void Manipulator::perpendiculars(const glm::vec3& direction, glm::vec3* first, glm::vec3* second) {
    // cross with whichever world axis the direction is least aligned with, so the
    // product is never degenerate
    glm::vec3 other(1.0f, 0.0f, 0.0f);
    if (std::fabs(direction.x) > std::fabs(direction.y)) {
        other = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    const glm::vec3 u = glm::normalize(glm::cross(direction, other));
    const glm::vec3 v = glm::normalize(glm::cross(direction, u));
    if (first != nullptr) {
        *first = u;
    }
    if (second != nullptr) {
        *second = v;
    }
}

/**
 **/
float Manipulator::distanceToSegment(const glm::vec2& from, const glm::vec2& to, const glm::vec2& target) {
    const glm::vec2 along = to - from;
    const float length = glm::dot(along, along);
    if (length <= 0.0f) {
        return glm::distance(from, target);
    }
    float fraction = glm::dot(target - from, along) / length;
    if (fraction < 0.0f) {
        fraction = 0.0f;
    } else if (fraction > 1.0f) {
        fraction = 1.0f;
    }
    return glm::distance(from + along * fraction, target);
}

/**
 **/
float Manipulator::along(const ViewPort& view, const Placement& placement, Axis axis,
    const glm::vec2& from, const glm::vec2& to) const {
    const glm::vec3 unit = direction(placement, axis);
    glm::vec2 root;
    glm::vec2 tip;
    if (!project(view, placement.origin, &root) ||
        !project(view, placement.origin + unit * placement.size, &tip)) {
        return 0.0f;
    }

    const glm::vec2 screen = tip - root;
    const float length = glm::length(screen);
    // a handle pointing at the viewer projects to a point, and a drag along it means
    // nothing
    if (length < 1.0f) {
        return 0.0f;
    }
    return glm::dot(to - from, screen / length) * (placement.size / length);
}

/**
 **/
void Manipulator::marker(v3d::render::realtime::LineCanvas* canvas, const Placement& placement,
    const glm::vec3& at, float size, const glm::vec4& colour) const {
    const glm::vec3 corner(size, size, size);
    canvas->push();
    canvas->transform(glm::translate(glm::mat4(1.0f), at) * glm::mat4_cast(placement.orientation));
    canvas->box(-corner, corner, colour);
    canvas->pop();
}

/**
 **/
bool Manipulator::grab(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
    const glm::vec2& cursor, Axis* axis) const {
    const Placement seat = placement(mesh, view);
    if (!seat.valid) {
        return false;
    }

    glm::vec2 root;
    if (!project(view, seat.origin, &root)) {
        return false;
    }

    // the centre handle sits on top of where all three axes meet, so it is tested
    // first or an axis root would always win it
    if (glm::distance(root, cursor) <= tolerance * 1.5f) {
        if (axis != nullptr) {
            *axis = Axis::None;
        }
        return true;
    }

    const Axis axes[3] = { Axis::X, Axis::Y, Axis::Z };
    bool found = false;
    float nearest = 0.0f;
    for (const Axis candidate : axes) {
        glm::vec2 tip;
        if (!project(view, seat.origin + direction(seat, candidate) * seat.size, &tip)) {
            continue;
        }
        const float distance = distanceToSegment(root, tip, cursor);
        if (distance > tolerance) {
            continue;
        }
        if (found && distance >= nearest) {
            continue;
        }
        found = true;
        nearest = distance;
        if (axis != nullptr) {
            *axis = candidate;
        }
    }
    return found;
}

};  // namespace v3d::editor
