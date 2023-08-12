/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <glm/vec3.hpp>

namespace v3d::ecs::component {
    /**
     * An RGB Color
     **/
    class Color3 final {
    public:
        Color3(const float red, const float green, const float blue) noexcept;

        /**
         * Move constructor
         **/
        Color3(Color3&&) noexcept;

        /**
         * Default destructor
         **/
        ~Color3() noexcept = default;

        /**
         **/
        float red() const;

        /**
         **/
        float green() const;

        /**
         **/
        float blue() const;

        /**
         * Move assignment
         **/
        Color3& operator=(Color3&&) noexcept;
    private:
        glm::vec3 color_;
    };

};  // namespace v3d::ecs::component
