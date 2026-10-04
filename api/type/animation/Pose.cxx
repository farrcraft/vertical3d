/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Pose.h"

#include <cstddef>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

namespace v3d::type::animation {

Pose rest(const Skeleton& skeleton) {
    Pose pose;
    pose.joints.reserve(skeleton.joints.size());
    for (const Skeleton::Joint& joint : skeleton.joints) {
        pose.joints.push_back(Pose::Joint{ joint.translation, joint.rotation, joint.scale });
    }
    return pose;
}

Pose blend(const Pose& from, const Pose& to, float weight) {
    if (from.joints.size() != to.joints.size() || weight <= 0.0f) {
        return from;
    }
    if (weight >= 1.0f) {
        return to;
    }
    Pose mixed;
    mixed.joints.resize(from.joints.size());
    for (std::size_t joint = 0; joint < from.joints.size(); joint++) {
        const Pose::Joint& a = from.joints[joint];
        const Pose::Joint& b = to.joints[joint];
        mixed.joints[joint].translation = glm::mix(a.translation, b.translation, weight);
        mixed.joints[joint].rotation = glm::normalize(glm::slerp(a.rotation, b.rotation, weight));
        mixed.joints[joint].scale = glm::mix(a.scale, b.scale, weight);
    }
    return mixed;
}

void palette(const Skeleton& skeleton, const Pose& pose, std::vector<glm::mat4>* out) {
    out->resize(skeleton.joints.size());
    // a parent precedes its children, so its global matrix is ready by the time a child needs it
    for (std::size_t index = 0; index < skeleton.joints.size(); index++) {
        const Skeleton::Joint& joint = skeleton.joints[index];
        const Pose::Joint local = index < pose.joints.size()
            ? pose.joints[index]
            : Pose::Joint{ joint.translation, joint.rotation, joint.scale };
        const glm::mat4 matrix = glm::translate(glm::mat4(1.0f), local.translation) * glm::mat4_cast(local.rotation) *
            glm::scale(glm::mat4(1.0f), local.scale);
        const glm::mat4& above = joint.parent < 0 ? skeleton.root : (*out)[static_cast<std::size_t>(joint.parent)];
        (*out)[index] = above * matrix;
    }
    // done in a second pass, because a child reads its parent's global matrix and not its skinning one
    for (std::size_t index = 0; index < skeleton.joints.size(); index++) {
        (*out)[index] = (*out)[index] * skeleton.joints[index].inverseBind;
    }
}

};  // namespace v3d::type::animation
