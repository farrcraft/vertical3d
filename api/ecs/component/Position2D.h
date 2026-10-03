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
    Position2D(float x, float y) noexcept;

    /**
     * Copy constructor - a snapshot of the previous step is a copy, per ADR-0060
     **/
    Position2D(const Position2D& p) noexcept = default;

    /**
     * Move constructor
     **/
    Position2D(Position2D&& p) noexcept;

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
    Position2D& operator=(Position2D&& p) noexcept;

    /**
     * Copy assignment
     **/
    Position2D& operator=(const Position2D& p) noexcept = default;

 private:
    glm::vec2 position_;
};

/**
 * The position alpha of the way from one to the other, which is how ecs::interpolated draws
 * it between two simulation steps.
 **/
Position2D interpolate(const Position2D& from, const Position2D& to, float alpha);

};  // namespace v3d::ecs::component
