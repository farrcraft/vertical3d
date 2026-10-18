/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::type {

/**
 * Where a thing is, which way it is turned, and how large it is. An ecs entity and a
 * dag::Transform, such as an editor mesh, are both placed by it, so the composition is
 * written once.
 **/
struct Transform final {
    glm::vec3 position{0.0F};
    glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 scale{1.0F};

    /**
     * @return the model matrix, scaling first, then rotating, then moving into place. Scale
     *         goes first so that it acts along the thing's own axes. In the other order, a
     *         non-uniform scale would shear the rotated shape
     **/
    glm::mat4 matrix() const;
};

/**
 * The transform a fraction alpha of the way from one to the other. The rotation is slerped,
 * so it turns the short way round.
 **/
Transform interpolate(const Transform& from, const Transform& to, float alpha);

};  // namespace v3d::type
