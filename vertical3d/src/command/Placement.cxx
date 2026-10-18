/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Placement.h"

#include <cmath>

namespace v3d::editor {

namespace {

/**
 * How far apart two placements may be and still be the same one. A drag is measured
 * in world units of a scene whose primitives are one unit across.
 **/
const float tolerance = 1e-6f;

/**
 **/
bool close(float first, float second) {
    return std::fabs(first - second) <= tolerance;
}

/**
 **/
bool close(const glm::vec3& first, const glm::vec3& second) {
    return close(first.x, second.x) && close(first.y, second.y) && close(first.z, second.z);
}

/**
 **/
bool close(const glm::quat& first, const glm::quat& second) {
    return close(first.w, second.w) && close(first.x, second.x) && close(first.y, second.y) && close(first.z, second.z);
}

};  // namespace

/**
 **/
Placement Placement::of(const v3d::dag::Transform& transform) {
    Placement placement;
    placement.translation = transform.translation();
    placement.rotation = transform.rotation();
    placement.scale = transform.scale();
    return placement;
}

/**
 **/
void Placement::applyTo(v3d::dag::Transform* transform) const {
    if (transform == nullptr) {
        return;
    }
    transform->translation(translation);
    transform->rotation(rotation);
    transform->scale(scale);
}

/**
 **/
bool Placement::same(const Placement& other) const {
    return close(translation, other.translation) &&
        close(rotation, other.rotation) &&
        close(scale, other.scale);
}

};  // namespace v3d::editor
