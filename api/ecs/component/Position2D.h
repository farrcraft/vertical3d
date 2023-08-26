/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <glm/vec2.hpp>

namespace v3d::ecs::component {
    /**
     * A 2D position
     **/
    class Position2D final {
    public:
        Position2D(const float x, const float y) noexcept;

        /**
         * Move constructor
         **/
        Position2D(Position2D&&) noexcept;

        /**
         * Default destructor
         **/
        ~Position2D() noexcept = default;

        /**
         **/
        float x() const;

        /**
         **/
        float y() const;

        /**
         **/
        glm::vec2 value() const;

        /**
         **/
        void set(const glm::vec2& position);

        /**
         * Move assignment
         **/
        Position2D& operator=(Position2D&&) noexcept;
    private:
        glm::vec2 position_;
    };

};  // namespace v3d::ecs::component
