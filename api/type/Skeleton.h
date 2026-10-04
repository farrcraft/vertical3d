/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::type {

/**
 * The joints a skinned model is bent by - ADR-0069.
 *
 * Joints are held in an order where every parent precedes its children, so that a pose can be
 * made global in one pass from the front. A joint's rest pose is local to its parent, or to
 * root for a joint with none.
 **/
struct Skeleton final {
    struct Joint final {
        std::string name;
        int32_t parent{ -1 };  /**< an index into joints, always lower than this one's, or -1 **/
        glm::vec3 translation{ 0.0f };
        glm::quat rotation = glm::identity<glm::quat>();
        glm::vec3 scale{ 1.0f };

        /**
         * Takes a vertex from the space the model's vertices are in to this joint's own, as
         * the joint stood when the model was bound to it.
         **/
        glm::mat4 inverseBind{ 1.0f };
    };

    std::vector<Joint> joints;

    /**
     * Where the root joints stand: whatever the file put above the skeleton, such as an
     * armature's scale, which no clip animates.
     **/
    glm::mat4 root{ 1.0f };

    /**
     * @return whether there is a skeleton at all, which a static model does not have
     **/
    bool empty() const noexcept;
};

};  // namespace v3d::type
