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
 * Where a thing is, which way it is turned, and how large it is - what an ecs entity and an
 * editor mesh are both placed by, so that the composition is written once.
 **/
struct Transform final {
    glm::vec3 position{0.0f};
    glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 scale{1.0f};

    /**
     * @return the model matrix, scaling first, then rotating, then moving into place. Scale
     *         goes first so that it acts along the thing's own axes: composed the other way
     *         round, a non-uniform scale shears everything the rotation turned
     **/
    glm::mat4 matrix() const;
};

/**
 * The transform alpha of the way from one to the other. The rotation is slerped, so it turns
 * the short way round.
 **/
Transform interpolate(const Transform& from, const Transform& to, float alpha);

};  // namespace v3d::type
