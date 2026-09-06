/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "IsometricCamera.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

namespace v3d::type {

namespace {

constexpr glm::vec3 UP(0.0f, 1.0f, 0.0f);

/**
 * A quarter turn per azimuth step.
 **/
constexpr float QUARTER_TURN = 1.57079632679489661923f;

/**
 * The step index reduced into [0, AZIMUTHS), for a negative index as well as a positive
 * one - the built in remainder keeps the sign of its left operand.
 **/
int wrap(int index) {
    const int remainder = index % IsometricCamera::AZIMUTHS;
    return remainder < 0 ? remainder + IsometricCamera::AZIMUTHS : remainder;
}

};  // namespace

IsometricCamera::IsometricCamera() : target_(0.0f, 0.0f, 0.0f) {
}

void IsometricCamera::rotate(int steps) {
    azimuth_ = wrap(azimuth_ + steps);
}

int IsometricCamera::azimuth() const {
    return azimuth_;
}

void IsometricCamera::azimuth(int index) {
    azimuth_ = wrap(index);
}

void IsometricCamera::pan(const glm::vec2& delta) {
    target_ += right() * delta.x + forward() * delta.y;
}

glm::vec3 IsometricCamera::forward() const {
    const float angle = static_cast<float>(azimuth_) * QUARTER_TURN;
    // the eye sits at +x of the target at azimuth zero, so the view runs the other way
    return glm::vec3(-std::cos(angle), 0.0f, -std::sin(angle));
}

glm::vec3 IsometricCamera::right() const {
    // the basis a CameraProfile builds is right = up x direction, and the projection sends
    // its x to the right of the screen. Taking the cross product rather than naming the
    // vector keeps this the same hand as whatever the profile does
    return glm::normalize(glm::cross(UP, forward()));
}

glm::vec3 IsometricCamera::eye() const {
    const float angle = static_cast<float>(azimuth_) * QUARTER_TURN;
    const float horizontal = std::cos(elevation_);
    const glm::vec3 offset(horizontal * std::cos(angle), std::sin(elevation_), horizontal * std::sin(angle));
    return target_ + offset * distance_;
}

glm::vec3 IsometricCamera::target() const {
    return target_;
}

void IsometricCamera::target(const glm::vec3& target) {
    target_ = target;
}

float IsometricCamera::zoom() const {
    return zoom_;
}

void IsometricCamera::zoom(float halfHeight) {
    zoom_ = std::clamp(halfHeight, MINIMUM_ZOOM, MAXIMUM_ZOOM);
}

void IsometricCamera::zoomBy(float delta) {
    zoom(zoom_ + delta);
}

float IsometricCamera::elevation() const {
    return elevation_;
}

void IsometricCamera::elevation(float radians) {
    elevation_ = radians;
}

float IsometricCamera::distance() const {
    return distance_;
}

void IsometricCamera::distance(float distance) {
    distance_ = distance;
}

void IsometricCamera::apply(Camera* camera) const {
    if (camera == nullptr) {
        return;
    }
    CameraProfile& profile = camera->profile();
    profile.orthographic(true);
    profile.orthoZoom(zoom_);
    profile.eye(eye());
    // up before lookat: lookat derives the other two normals and the rotation from it
    profile.up(UP);
    profile.lookat(target_);
}

};  // namespace v3d::type
