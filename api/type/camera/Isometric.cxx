/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Isometric.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

namespace v3d::type::camera {

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
    const int remainder = index % Isometric::AZIMUTHS;
    return remainder < 0 ? remainder + Isometric::AZIMUTHS : remainder;
}

};  // namespace

Isometric::Isometric() : target_(0.0f, 0.0f, 0.0f) {
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

void Isometric::pan(const glm::vec2& delta) {
    target_ += right() * delta.x + forward() * delta.y;
}

glm::vec3 Isometric::forward() const {
    const float angle = static_cast<float>(azimuth_) * QUARTER_TURN;
    // the eye sits at +x of the target at azimuth zero, so the view runs the other way
    return glm::vec3(-std::cos(angle), 0.0f, -std::sin(angle));
}

glm::vec3 Isometric::right() const {
    // the projection sends the basis x to the right of the screen, so this is the profile's
    // right flattened onto the ground - crossed the way the hand names rather than the way
    // this tree's default does, which is the whole of what a hand decides
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
    const float angle = static_cast<float>(azimuth_) * QUARTER_TURN;
    const float horizontal = std::cos(elevation_);
    const glm::vec3 offset(horizontal * std::cos(angle), std::sin(elevation_), horizontal * std::sin(angle));
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
    // before lookat, which is what reads it
    profile.hand(hand_);
    profile.orthographic(true);
    profile.orthoZoom(zoom_);
    profile.eye(eye());
    // up before lookat: lookat derives the other two normals and the rotation from it
    profile.up(UP);
    profile.lookat(target_);
}

};  // namespace v3d::type::camera
