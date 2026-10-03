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
    Position1D(float x) noexcept;

    /**
     * Copy constructor - a snapshot of the previous step is a copy, per ADR-0060
     **/
    Position1D(const Position1D& p) noexcept = default;

    /**
     * Move constructor
     **/
    Position1D(Position1D&& p) noexcept;

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
    Position1D& operator=(Position1D&& p) noexcept;

    /**
     * Copy assignment
     **/
    Position1D& operator=(const Position1D& p) noexcept = default;

 private:
    float position_;
};

/**
 * The position alpha of the way from one to the other, which is how ecs::interpolated draws
 * it between two simulation steps.
 **/
Position1D interpolate(const Position1D& from, const Position1D& to, float alpha);

};  // namespace v3d::ecs::component
