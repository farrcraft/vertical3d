/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <glm/vec2.hpp>

namespace v3d::component {
    /**
     **/
    class Position final {
     public:
        Position(const int x, const int y) noexcept;

        /**
         * Move constructor
         **/
        Position(Position&&) noexcept;

        /**
         * Default destructor
         **/
        ~Position() noexcept = default;

        /**
         * Move assignment
         **/
        Position& operator=(Position&&) noexcept;
     private:
        glm::ivec2 position_;
    };

};  // namespace v3d::component
