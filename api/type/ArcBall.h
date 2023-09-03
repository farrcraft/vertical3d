/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

namespace v3d::type {
    /**
     * ArcBall camera rotation utility.
     */
    class ArcBall final {
     public:
        ArcBall();
        ~ArcBall();

        void click(const glm::vec2 & point);
        glm::quat drag(const glm::vec2 & point);
        glm::vec3 map(const glm::vec2 & new_point);
        void bounds(float width, float height);

     private:
        glm::vec3 start_;
        glm::vec3 end_;
        float width_;
        float height_;
    };

};  // namespace v3d::type
