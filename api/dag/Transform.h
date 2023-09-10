/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

namespace v3d::dag {
    class Transform {
     public:
        Transform();
        virtual ~Transform();

        void scale(const glm::vec3 & s);
        void rotation(const glm::quat & r);
        virtual void translation(const glm::vec3 & t);

        glm::vec3 scale(void) const;
        glm::quat rotation(void) const;
        virtual glm::vec3 translation(void) const;

        glm::mat4 matrix(void) const;

     private:
        glm::vec3 translation_;
        glm::vec3 scale_;
        glm::quat rotation_;
    };

};  // namespace v3d::dag
