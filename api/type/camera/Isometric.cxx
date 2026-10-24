/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Isometric.h"

#include "Camera.h"
#include "Profile.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

namespace v3d::type::camera {

namespace {

constexpr glm::vec3 UP(0.0F, 1.0F, 0.0F);

/**
 * A quarter turn per azimuth step.
 **/
constexpr float QUARTER_TURN = 1.57079632679489661923F;

/**
 * The step index reduced into [0, AZIMUTHS), for a negative index as well as a positive
 * one. The built in remainder keeps the sign of its left operand.
 **/
int wrap(int index) {
    const int remainder = index % Isometric::AZIMUTHS;
    return remainder < 0 ? remainder + Isometric::AZIMUTHS : remainder;
}

};  // namespace

Isometric::Isometric() : target_(0.0F, 0.0F, 0.0F) {
}

void Isometric::rotate(int steps) {
    azimuth_ = wrap(azimuth_ + steps);
}

int Isometric::azimuth() const {
    return azimuth_;
}

void Isometric::azimuth(int index) {
    azimuth_ = wrap(index);
}

float Isometric::start() const {
    return start_;
}

void Isometric::start(float radians) {
    start_ = radians;
}

float Isometric::angle() const {
    return start_ + (static_cast<float>(azimuth_) * QUARTER_TURN);
}

void Isometric::pan(const glm::vec2& delta) {
    target_ += right() * delta.x + forward() * delta.y;
}

glm::vec3 Isometric::forward() const {
    const float turn = angle();
    // the view runs from the eye towards the target, the opposite of the eye's offset
    return glm::vec3(-std::cos(turn), 0.0F, -std::sin(turn));
}

glm::vec3 Isometric::right() const {
    // the profile's right flattened onto the ground. The hand changes only the order of
    // the cross product
    return hand_ == Profile::Hand::DirectionCrossUp
        ? glm::normalize(glm::cross(forward(), UP))
        : glm::normalize(glm::cross(UP, forward()));
}

Profile::Hand Isometric::hand() const {
    return hand_;
}

void Isometric::hand(Profile::Hand hand) {
    hand_ = hand;
}

glm::vec3 Isometric::eye() const {
    const float turn = angle();
    const float horizontal = std::cos(elevation_);
    const glm::vec3 offset(horizontal * std::cos(turn), std::sin(elevation_), horizontal * std::sin(turn));
    return target_ + offset * distance_;
}

glm::vec3 Isometric::target() const {
    return target_;
}

void Isometric::target(const glm::vec3& target) {
    target_ = target;
}

float Isometric::zoom() const {
    return zoom_;
}

void Isometric::zoom(float halfHeight) {
    zoom_ = std::clamp(halfHeight, MINIMUM_ZOOM, MAXIMUM_ZOOM);
}

void Isometric::zoomBy(float delta) {
    zoom(zoom_ + delta);
}

float Isometric::elevation() const {
    return elevation_;
}

void Isometric::elevation(float radians) {
    elevation_ = radians;
}

float Isometric::distance() const {
    return distance_;
}

void Isometric::distance(float distance) {
    distance_ = distance;
}

void Isometric::apply(Camera* camera) const {
    if (camera == nullptr) {
        return;
    }
    Profile& profile = camera->profile();
    // before lookat(), which reads it
    profile.hand(hand_);
    profile.orthographic(true);
    profile.orthoZoom(zoom_);
    profile.eye(eye());
    // up before lookat(), which derives the other two normals and the rotation from it
    profile.up(UP);
    profile.lookat(target_);
}

};  // namespace v3d::type::camera
