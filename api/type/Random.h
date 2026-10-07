/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::type {

/**
 * A deterministic source of random numbers, whose whole state is one integer.
 *
 * The generator is splitmix64, and every seed is valid, including zero. state() is also a seed
 * that resumes the sequence where it stands. A save stores state(), and a load passes it to
 * the constructor.
 *
 * Every value is derived here rather than through <random>, whose distributions are
 * implementation-defined: a seed gives the same floats under any standard library, so a test
 * can pin them.
 *
 * Not thread-safe. Its output depends on the order of the calls.
 **/
class Random final {
 public:
    explicit Random(uint64_t seed) noexcept;

    /**
     * @return the next 64 bits of the sequence
     **/
    uint64_t next() noexcept;

    /**
     * @return a whole number in [0, bound), with no modulo bias
     * @throw std::invalid_argument for a bound of zero
     **/
    uint32_t below(uint32_t bound);

    /**
     * @return a float in [0, 1), from the top 24 bits of the next value, so that every float it
     *         gives is equally likely and one is never reached
     **/
    float unit() noexcept;

    /**
     * When low + (high - low) * unit() rounds up to high, the largest float below high is
     * returned instead. A span with high not above low is returned unchecked.
     *
     * @return a float in [low, high) when high > low
     **/
    float range(float low, float high) noexcept;

    /**
     * @return a point in the box spanned by two corners, each axis drawn in turn
     **/
    glm::vec3 inside(const glm::vec3& minimum, const glm::vec3& maximum) noexcept;

    /**
     * A direction spread evenly over the cap of the unit sphere within an angle of an axis.
     *
     * @param axis the cone's centre, which need not be normalised but must not be zero
     * @param angle the half angle in radians: zero is the axis, pi the whole sphere
     * @return a unit direction
     **/
    glm::vec3 cone(const glm::vec3& axis, float angle) noexcept;

    /**
     * @return the generator's state, which is also a seed that resumes the sequence from here
     **/
    uint64_t state() const noexcept;

 private:
    uint64_t state_;
};

};  // namespace v3d::type
