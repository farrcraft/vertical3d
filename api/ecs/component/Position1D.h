/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec2.hpp>

namespace v3d::ecs::component {
    /**
     * A 1D position
     **/
    class Position1D final {
     public:
        Position1D(const float x) noexcept;

        /**
         * Move constructor
         **/
        Position1D(Position1D&&) noexcept;

        /**
         * Default destructor
         **/
        ~Position1D() noexcept = default;

        /**
         **/
        float x() const;

        /**
         **/
        float value() const;

        /**
         **/
        void set(float pos);

        /**
         * Move assignment
         **/
        Position1D& operator=(Position1D&&) noexcept;

     private:
        float position_;
    };

};  // namespace v3d::ecs::component
