/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#include "CameraController.h"

#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>

#include <algorithm>
#include <cmath>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>

namespace {

/** Degrees below or above the horizon the view stops at: straight down or straight up. **/
constexpr float PITCH_LIMIT = 90.0F;

const glm::vec3 WORLD_UP(0.0F, 1.0F, 0.0F);

/** One axis's velocity, which way it is driven, how fast it changes, its limit, and the time. **/
struct Axis {
    float current = 0.0F;
    float direction = 0.0F;
    float acceleration = 0.0F;
    float limit = 0.0F;
    float elapsed = 0.0F;
};

/**
 * One axis of the velocity, after one update.
 *
 * A driven axis accelerates toward its limit and is clamped there. An axis that is not
 * driven decelerates toward zero and stops there, without crossing it. The deceleration
 * therefore takes the sign that moves the velocity toward zero.
 **/
float axisVelocity(const Axis& axis) {
    float current = axis.current;
    if (axis.direction != 0.0F) {
        current += axis.direction * axis.acceleration * axis.elapsed;
        return std::clamp(current, -axis.limit, axis.limit);
    }
    if (current > 0.0F) {
        current -= axis.acceleration * axis.elapsed;
        return current < 0.0F ? 0.0F : current;
    }
    current += axis.acceleration * axis.elapsed;
    return current > 0.0F ? 0.0F : current;
}

};  // namespace

CameraController::CameraController(v3d::type::camera::Camera* camera, const Motion& motion) :
    camera_(camera),
    motion_(motion) {
    // right = direction x up, as glm::lookAt builds it. The rotation then needs half a turn
    // about up to look down -z
    camera_->profile().hand(v3d::type::camera::Profile::Hand::DirectionCrossUp);
    camera_->profile().eye(glm::vec3(0.0F));
    perspective(Lens());
    update();
}

void CameraController::perspective(const Lens& lens) {
    v3d::type::camera::Profile& profile = camera_->profile();
    // the profile holds the vertical field of view. The one that keeps the horizontal field at
    // this aspect has a half angle whose tangent is the horizontal one's over the aspect
    const float halfAcross = glm::radians(lens.fovx) / 2.0F;
    profile.fov(glm::degrees(2.0F * std::atan(std::tan(halfAcross) / lens.aspect)));
    profile.pixelAspect(lens.aspect);
    profile.clipping(glm::vec2(lens.znear, lens.zfar));
    profile.orthographic(false);
    camera_->createProjection();
}

void CameraController::look(float headingDegrees, float pitchDegrees) {
    heading_ += headingDegrees;
    pitch_ = std::clamp(pitch_ + pitchDegrees, -PITCH_LIMIT, PITCH_LIMIT);
    update();
}

glm::vec3 CameraController::displacement(const glm::vec3& direction, float step) const {
    glm::vec3 displacement(0.0F);
    if (velocity_ == glm::vec3(0.0F)) {
        return displacement;
    }
    displacement = (velocity_ * step) + (0.5F * motion_.acceleration * step * step);
    for (glm::length_t axis = 0; axis < 3; axis++) {
        if (direction[axis] == 0.0F && glm::epsilonEqual(velocity_[axis], 0.0F, glm::epsilon<float>())) {
            displacement[axis] = 0.0F;
        }
    }
    return displacement;
}

glm::vec3 CameraController::moved(const glm::vec3& displacement) const {
    const float heading = glm::radians(heading_);
    const glm::vec3 right(std::cos(heading), 0.0F, std::sin(heading));
    const glm::vec3 ahead(std::sin(heading), 0.0F, -std::cos(heading));
    return camera_->profile().eye() + (right * displacement.x) + (WORLD_UP * displacement.y) + (ahead * displacement.z);
}

void CameraController::accelerate(const glm::vec3& direction, float step) {
    for (glm::length_t axis = 0; axis < 3; axis++) {
        velocity_[axis] = axisVelocity({.current = velocity_[axis], .direction = direction[axis],
            .acceleration = motion_.acceleration[axis], .limit = motion_.limit[axis], .elapsed = step});
    }
}

void CameraController::place(const glm::vec3& eye) {
    camera_->profile().eye(eye);
    update();
}

const glm::vec3& CameraController::velocity() const noexcept {
    return velocity_;
}

void CameraController::update() {
    // the profile looks along +z of its rotation. Half a turn about up less the heading points
    // that down -z and turns it right, and the pitch about the camera's own x axis tips it down
    const glm::quat heading = glm::angleAxis(glm::pi<float>() - glm::radians(heading_), WORLD_UP);
    const glm::quat pitch = glm::angleAxis(glm::radians(pitch_), glm::vec3(1.0F, 0.0F, 0.0F));
    v3d::type::camera::Profile& profile = camera_->profile();
    profile.rotation(glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    profile.turn(heading * pitch);
    camera_->createView();
}
