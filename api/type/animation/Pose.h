/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/Skeleton.h>

#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::type::animation {

/**
 * Every joint's translation, rotation and scale, local to its parent, in the skeleton's order.
 * Sampling a clip at a time gives one.
 **/
struct Pose final {
    struct Joint final {
        glm::vec3 translation{ 0.0f };
        glm::quat rotation = glm::identity<glm::quat>();
        glm::vec3 scale{ 1.0f };
    };

    std::vector<Joint> joints;
};

/**
 * @return the skeleton standing in its rest pose, the base a clip that animates only some
 *         joints is sampled over
 **/
Pose rest(const Skeleton& skeleton);

/**
 * Two poses mixed: translations and scales lerped, rotations slerped the short way round. A
 * weight of 0 is the first exactly and 1 the second. Poses of different sizes give the first.
 **/
Pose blend(const Pose& from, const Pose& to, float weight);

/**
 * The matrices a vertex is skinned by: each joint made global through its parents and the
 * skeleton's root, then times its inverse bind. A skeleton at the pose it was bound in gives
 * the identity for every joint.
 *
 * @param out resized to the skeleton's joints, so a caller can reuse one across frames
 **/
void palette(const Skeleton& skeleton, const Pose& pose, std::vector<glm::mat4>* out);

};  // namespace v3d::type::animation
