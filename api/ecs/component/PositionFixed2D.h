/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec2.hpp>

namespace v3d::ecs::component {
    /**
     * A 2D position in integer space
     **/
    class PositionFixed2D final {
     public:
         PositionFixed2D(const int x, const int y) noexcept;

        /**
         * Move constructor
         **/
         PositionFixed2D(PositionFixed2D&&) noexcept;

        /**
         * Default destructor
         **/
        ~PositionFixed2D() noexcept = default;

        /**
         **/
        int x() const;

        /**
         **/
        int y() const;

        /**
         * Move assignment
         **/
        PositionFixed2D& operator=(PositionFixed2D&&) noexcept;

     private:
        glm::ivec2 position_;
    };

};  // namespace v3d::ecs::component
