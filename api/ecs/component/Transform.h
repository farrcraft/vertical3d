/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::ecs::component {

/**
 * Where a thing stands in a 3D world, which way it is turned, and how large it is - ADR-0063.
 *
 * The position is the thing's origin: a sprite's feet, a mesh's own origin. The fields are
 * written directly, since simulation sets them every step.
 **/
struct Transform final {
    glm::vec3 position{0.0f};
    glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 scale{1.0f};

    /**
     * @return the model matrix, scaling first, then rotating, then moving into place
     **/
    glm::mat4 matrix() const;
};

/**
 * A turn about +Y, for a world that turns about one axis. A positive angle turns +Z towards
 * +X.
 **/
glm::quat aboutY(float radians);

/**
 * The transform alpha of the way from one to the other, which is how ecs::interpolated draws
 * it between two simulation steps. The rotation is slerped, so it turns the short way round.
 **/
Transform interpolate(const Transform& from, const Transform& to, float alpha);

};  // namespace v3d::ecs::component
