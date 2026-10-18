/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Random.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

namespace v3d::type {

namespace {

// SplitMix64: the counter's step, which is 2^64 divided by the golden ratio, and the shifts
// and multipliers of its two mixing rounds and final fold
constexpr uint64_t STATE_STEP = 0x9E3779B97F4A7C15ULL;
constexpr int FIRST_MIX_SHIFT = 30;
constexpr uint64_t FIRST_MIX_MULTIPLIER = 0xBF58476D1CE4E5B9ULL;
constexpr int SECOND_MIX_SHIFT = 27;
constexpr uint64_t SECOND_MIX_MULTIPLIER = 0x94D049BB133111EBULL;
constexpr int FOLD_SHIFT = 31;

// unit() keeps the top 24 bits of a draw, the precision of a float, and scales them by 2^-24
constexpr int UNIT_DISCARDED_BITS = 40;
constexpr float UNIT_SCALE = 0x1.0p-24F;

};  // namespace

Random::Random(uint64_t seed) noexcept :
    state_(seed) {
}

uint64_t Random::next() noexcept {
    // the state is only a counter and the mixing randomises the output, so the generator
    // saves and restores as one integer
    state_ += STATE_STEP;
    uint64_t z = state_;
    z = (z ^ (z >> FIRST_MIX_SHIFT)) * FIRST_MIX_MULTIPLIER;
    z = (z ^ (z >> SECOND_MIX_SHIFT)) * SECOND_MIX_MULTIPLIER;
    return z ^ (z >> FOLD_SHIFT);
}

uint32_t Random::below(uint32_t bound) {
    if (bound == 0) {
        throw std::invalid_argument("Random::below needs a positive bound");
    }
    // Lemire's multiply and shift, with the rejection that removes the bias next() % bound
    // would have whenever bound does not divide 2^32
    auto draw = [this, bound] { return static_cast<uint64_t>(static_cast<uint32_t>(next())) * bound; };
    uint64_t product = draw();
    if (static_cast<uint32_t>(product) < bound) {
        // 2^32 % bound, without a 64-bit divide
        const uint32_t threshold = (~bound + 1U) % bound;
        while (static_cast<uint32_t>(product) < threshold) {
            product = draw();
        }
    }
    return static_cast<uint32_t>(product >> 32);
}

float Random::unit() noexcept {
    return static_cast<float>(next() >> UNIT_DISCARDED_BITS) * UNIT_SCALE;
}

float Random::range(float low, float high) noexcept {
    const float value = low + ((high - low) * unit());
    // the sum rounds to high when the span is small beside low, so step back below it
    if (high > low && value >= high) {
        return std::nextafter(high, low);
    }
    return value;
}

glm::vec3 Random::inside(const glm::vec3& minimum, const glm::vec3& maximum) noexcept {
    const float x = range(minimum.x, maximum.x);
    const float y = range(minimum.y, maximum.y);
    const float z = range(minimum.z, maximum.z);
    return glm::vec3(x, y, z);
}

glm::vec3 Random::cone(const glm::vec3& axis, float angle) noexcept {
    // a height drawn evenly between the cap's rim and its pole spreads directions evenly over
    // the cap's area, by Archimedes' hat-box theorem
    const float height = 1.0F - (unit() * (1.0F - std::cos(angle)));
    const float around = unit() * glm::two_pi<float>();
    const float radius = std::sqrt(std::max(0.0F, 1.0F - (height * height)));

    const glm::vec3 pole = glm::normalize(axis);
    const glm::vec3 other = std::abs(pole.x) < 0.9F ? glm::vec3(1.0F, 0.0F, 0.0F) : glm::vec3(0.0F, 1.0F, 0.0F);
    const glm::vec3 first = glm::normalize(glm::cross(pole, other));
    const glm::vec3 second = glm::cross(pole, first);
    return (pole * height) + (first * (radius * std::cos(around))) + (second * (radius * std::sin(around)));
}

uint64_t Random::state() const noexcept {
    return state_;
}

};  // namespace v3d::type
