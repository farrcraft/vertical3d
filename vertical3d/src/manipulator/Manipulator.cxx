/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Manipulator.h"

#include <api/brep/BRep.h>
#include <api/render/realtime/canvas/Line.h>
#include <api/type/camera/Camera.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cmath>
#include <cstddef>

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
constexpr glm::vec4 xColour(0.90F, 0.28F, 0.32F, 1.0F);
constexpr glm::vec4 yColour(0.42F, 0.82F, 0.35F, 1.0F);
constexpr glm::vec4 zColour(0.32F, 0.55F, 0.95F, 1.0F);
constexpr glm::vec4 centreColour(0.80F, 0.82F, 0.86F, 1.0F);
constexpr glm::vec4 activeColour(1.0F, 0.86F, 0.25F, 1.0F);

};  // namespace

const float Manipulator::handlePixels = 90.0F;
const float Manipulator::tolerance = 6.0F;

/**
 **/
Manipulator::Placement::Placement() :
valid(false),
origin(0.0F, 0.0F, 0.0F),
orientation(1.0F, 0.0F, 0.0F, 0.0F),
size(0.0F) {
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
Manipulator::Placement Manipulator::placement(const Meshes& meshes,
    const ViewPort& view) const {
    Placement placement;
    glm::vec3 sum(0.0F, 0.0F, 0.0F);
    boost::shared_ptr<v3d::brep::BRep> first;
    std::size_t count = 0;
    for (const boost::shared_ptr<v3d::brep::BRep>& mesh : meshes) {
        if (!mesh) {
            continue;
        }
        if (!first) {
            first = mesh;
        }
        sum += mesh->translation();
        count++;
    }
    if (count == 0) {
        return placement;
    }

    const boost::shared_ptr<v3d::type::camera::Camera> camera = view.camera();
    const glm::vec4& region = view.region();
    if (!camera || region.z <= 0.0F || region.w <= 0.0F) {
        return placement;
    }

    // the camera as it stands, not as it was last drawn: a drag arrives between frames
    camera->createProjection();
    camera->createView();

    placement.origin = sum / static_cast<float>(count);
    placement.orientation = effective() == Space::Local ? first->rotation() : glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
    placement.size = handlePixels * unitsPerPixel(view, placement.origin);
    placement.valid = placement.size > 0.0F;
    return placement;
}

/**
 **/
glm::vec3 Manipulator::direction(const Placement& placement, Axis axis) {
    glm::vec3 unit(0.0F, 0.0F, 0.0F);
    switch (axis) {
    case Axis::X:
        unit.x = 1.0F;
        break;
    case Axis::Y:
        unit.y = 1.0F;
        break;
    case Axis::Z:
        unit.z = 1.0F;
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
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view.camera();
    const glm::vec4& region = view.region();
    if (!camera || region.z <= 0.0F || region.w <= 0.0F) {
        return false;
    }

    int viewport[4];
    viewport[0] = static_cast<int>(region.x);
    viewport[1] = static_cast<int>(region.y);
    viewport[2] = static_cast<int>(region.z);
    viewport[3] = static_cast<int>(region.w);

    const glm::vec3 projected = camera->project(point, viewport);
    // outside the depth range is behind the near plane or beyond the far one. A point
    // behind a perspective camera lands outside the range too, so it is not taken for a
    // point in front
    if (projected.z < 0.0F || projected.z > 1.0F) {
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
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view.camera();
    const glm::vec4& region = view.region();
    if (!camera || region.z <= 0.0F || region.w <= 0.0F) {
        return 0.0F;
    }

    int viewport[4];
    viewport[0] = static_cast<int>(region.x);
    viewport[1] = static_cast<int>(region.y);
    viewport[2] = static_cast<int>(region.z);
    viewport[3] = static_cast<int>(region.w);

    const glm::vec3 projected = camera->project(at, viewport);
    const glm::vec3 here = camera->unproject(projected, viewport);
    const glm::vec3 next = camera->unproject(glm::vec3(projected.x + 1.0F, projected.y, projected.z), viewport);
    return glm::length(next - here);
}

/**
 **/
Manipulator::Basis Manipulator::basis(const ViewPort& view) {
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view.camera();
    if (!camera) {
        return Basis();
    }
    // the view matrix rotates the world into the camera's axes, so its rows are those
    // axes in world space
    const glm::mat4 matrix = camera->view();
    return {
        .right = glm::vec3(matrix[0][0], matrix[1][0], matrix[2][0]),
        .up = glm::vec3(matrix[0][1], matrix[1][1], matrix[2][1]),
        .forward = glm::vec3(matrix[0][2], matrix[1][2], matrix[2][2]),
    };
}

/**
 **/
void Manipulator::perpendiculars(const glm::vec3& direction, glm::vec3* first, glm::vec3* second) {
    // cross with whichever world axis the direction is least aligned with, so the
    // product is never degenerate
    glm::vec3 other(1.0F, 0.0F, 0.0F);
    if (std::fabs(direction.x) > std::fabs(direction.y)) {
        other = glm::vec3(0.0F, 1.0F, 0.0F);
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
    if (length <= 0.0F) {
        return glm::distance(from, target);
    }
    float fraction = glm::dot(target - from, along) / length;
    if (fraction < 0.0F) {
        fraction = 0.0F;
    } else if (fraction > 1.0F) {
        fraction = 1.0F;
    }
    return glm::distance(from + along * fraction, target);
}

/**
 **/
float Manipulator::along(const ViewPort& view, const Placement& placement, Axis axis,
    const glm::vec2& from, const glm::vec2& to) {
    const glm::vec3 unit = direction(placement, axis);
    glm::vec2 root;
    glm::vec2 tip;
    if (!project(view, placement.origin, &root) ||
        !project(view, placement.origin + unit * placement.size, &tip)) {
        return 0.0F;
    }

    const glm::vec2 screen = tip - root;
    const float length = glm::length(screen);
    // a handle pointing at the viewer projects to a point, which has no direction to
    // measure a drag along
    if (length < 1.0F) {
        return 0.0F;
    }
    return glm::dot(to - from, screen / length) * (placement.size / length);
}

/**
 **/
void Manipulator::marker(v3d::render::realtime::canvas::Line* canvas, const Placement& placement,
    const glm::vec3& at, float size, const glm::vec4& colour) {
    const glm::vec3 corner(size, size, size);
    canvas->push();
    canvas->transform(glm::translate(glm::mat4(1.0F), at) * glm::mat4_cast(placement.orientation));
    canvas->box(-corner, corner, colour);
    canvas->pop();
}

/**
 **/
bool Manipulator::grab(const Meshes& meshes, const ViewPort& view,
    const glm::vec2& cursor, Axis* axis) const {
    const Placement seat = placement(meshes, view);
    if (!seat.valid) {
        return false;
    }

    glm::vec2 root;
    if (!project(view, seat.origin, &root)) {
        return false;
    }

    // the centre handle sits on top of where all three axes meet, so it is tested
    // first or an axis root would always win it
    if (glm::distance(root, cursor) <= tolerance * centreReach) {
        if (axis != nullptr) {
            *axis = Axis::None;
        }
        return true;
    }

    const Axis axes[3] = { Axis::X, Axis::Y, Axis::Z };
    bool found = false;
    float nearest = 0.0F;
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
