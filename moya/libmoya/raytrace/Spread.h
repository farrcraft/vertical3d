/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <algorithm>

#include <glm/vec3.hpp>

namespace v3d::moya::raytrace {

/**
 * How far a pixel's samples disagree: the variance of their mean, per channel, which RI's
 * PixelVariance bounds.
 **/
class Spread {
 public:
    void add(const glm::vec3 & colour) {
        count_++;
        sum_ += glm::dvec3(colour);
        squares_ += glm::dvec3(colour) * glm::dvec3(colour);
    }

    /**
     * Whether the largest channel's variance of the mean is within the bound. Fewer than two
     * samples give no measure of spread, so they are never settled.
     **/
    bool settled(float bound) const {
        if (count_ < 2) {
            return false;
        }
        const double samples = static_cast<double>(count_);
        const glm::dvec3 mean = sum_ / samples;
        const glm::dvec3 variance = (squares_ - mean * mean * samples) / (samples - 1.0);
        const double worst = std::max(variance.x, std::max(variance.y, variance.z));
        return worst / samples <= static_cast<double>(bound);
    }

 private:
    unsigned int count_ = 0;
    glm::dvec3 sum_ { 0.0 };
    glm::dvec3 squares_ { 0.0 };
};

};  // namespace v3d::moya::raytrace
