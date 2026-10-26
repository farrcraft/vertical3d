/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Sampling.h"

#include <api/type/Checked.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

namespace v3d::render::offline {

namespace {

/** The coefficients of a cubic in the radius, of radius^3, radius^2, radius and 1 in turn. **/
typedef std::array<float, 4> Cubic;

/**
 * The two pieces of the Catmull-Rom kernel, doubled: one for a radius under one, and one for a
 * radius from one to two. Doubling scales every weight alike, and the film divides by the total
 * weight.
 **/
constexpr Cubic CATMULL_ROM_INNER{3.0F, -5.0F, 0.0F, 2.0F};
constexpr Cubic CATMULL_ROM_OUTER{-1.0F, 5.0F, -8.0F, 4.0F};

/** A cubic at a radius, given the radius and its square. **/
float evaluate(const Cubic & cubic, float radius, float radiusSquared) {
    return (cubic[0] * radius * radiusSquared) + (cubic[1] * radiusSquared) + (cubic[2] * radius) + cubic[3];
}

};  // namespace

bool filterNamed(const std::string & name, Filter * filter) {
    if (name == "box") {
        *filter = Filter::Box;
    } else if (name == "triangle") {
        *filter = Filter::Triangle;
    } else if (name == "catmull-rom") {
        *filter = Filter::CatmullRom;
    } else if (name == "gaussian") {
        *filter = Filter::Gaussian;
    } else if (name == "sinc") {
        *filter = Filter::Sinc;
    } else {
        return false;
    }
    return true;
}

std::string filterName(Filter filter) {
    switch (filter) {
        case Filter::Box: return "box";
        case Filter::Triangle: return "triangle";
        case Filter::CatmullRom: return "catmull-rom";
        case Filter::Gaussian: return "gaussian";
        case Filter::Sinc: return "sinc";
    }
    return "gaussian";
}

float filter(Filter kind, const glm::vec2 & offset, const glm::vec2 & width) {
    const glm::vec2 half = width * 0.5F;
    if (!(std::fabs(offset.x) <= half.x) || !(std::fabs(offset.y) <= half.y)) {
        return 0.0F;
    }
    switch (kind) {
    case Filter::Box:
        return 1.0F;
    case Filter::Triangle:
        return (1.0F - (std::fabs(offset.x) / half.x)) * (1.0F - (std::fabs(offset.y) / half.y));
    case Filter::CatmullRom: {
        // radial and not scaled by the width. The kernel reaches zero two pixels out, so a
        // width under four cuts it off at the width test above. RI applies that cut-off to
        // every filter
        const float radiusSquared = (offset.x * offset.x) + (offset.y * offset.y);
        const float radius = std::sqrt(radiusSquared);
        if (radius >= 2.0F) {
            return 0.0F;
        }
        if (radius < 1.0F) {
            return evaluate(CATMULL_ROM_INNER, radius, radiusSquared);
        }
        return evaluate(CATMULL_ROM_OUTER, radius, radiusSquared);
    }
    case Filter::Gaussian: {
        // scaled so that the edge of the width is two standard deviations out
        const float x = offset.x * 2.0F / width.x;
        const float y = offset.y * 2.0F / width.y;
        return std::exp(-2.0F * ((x * x) + (y * y)));
    }
    case Filter::Sinc: {
        const float pi = 3.14159265358979F;
        const float x = offset.x == 0.0F ? 1.0F : std::sin(pi * offset.x) / (pi * offset.x);
        const float y = offset.y == 0.0F ? 1.0F : std::sin(pi * offset.y) / (pi * offset.y);
        return x * y;
    }
    }
    return 0.0F;
}

bool resolution(float side) {
    return v3d::type::toCount(side, {.minimum = 1, .maximum = largestResolution}).has_value();
}

std::optional<uint32_t> formatSide(float side) {
    if (side <= 0.0F && std::isfinite(side)) {
        return 0;
    }
    return v3d::type::toCount(side, {.minimum = 1, .maximum = largestResolution});
}

bool filterWidth(const glm::vec2 & width) {
    const bool wide = width.x > 0.0F && std::isfinite(width.x);
    const bool tall = width.y > 0.0F && std::isfinite(width.y);
    return wide && tall;
}

unsigned int sampleCount(float rate) {
    // the comparison is written so that a NaN takes one sample too
    if (!(rate >= 1.0F)) {
        return 1;
    }
    // capped so that the count fits the conversion, and a pixel's grid of samples fits an
    // unsigned count
    if (!(rate <= static_cast<float>(maximumSamples))) {
        return maximumSamples;
    }
    return static_cast<unsigned int>(std::lround(rate));  // checked: rate is in [1, maximumSamples], and a NaN returned above
}

float Sampling::lensRadius() const {
    return pinhole() ? 0.0F : focalLength / (2.0F * fstop);
}

bool Sampling::pinhole() const {
    return !(fstop > 0.0F) || std::isinf(fstop) || !(focalLength > 0.0F) || !(focalDistance > 0.0F);
}

};  // namespace v3d::render::offline
