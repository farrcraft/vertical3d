/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Sampling.h"

#include <api/type/Checked.h>

#include <cmath>
#include <string>

namespace v3d::render::offline {

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

float filter(Filter kind, const glm::vec2 & offset, const glm::vec2 & width) {
    const glm::vec2 half = width * 0.5f;
    if (!(std::fabs(offset.x) <= half.x) || !(std::fabs(offset.y) <= half.y)) {
        return 0.0f;
    }
    switch (kind) {
    case Filter::Box:
        return 1.0f;
    case Filter::Triangle:
        return (1.0f - std::fabs(offset.x) / half.x) * (1.0f - std::fabs(offset.y) / half.y);
    case Filter::CatmullRom: {
        // radial and not scaled by the width. The kernel reaches zero two pixels out, so a
        // width under four cuts it off at the box tested above, which is RI's rule for every
        // filter
        const float r2 = offset.x * offset.x + offset.y * offset.y;
        const float r = std::sqrt(r2);
        if (r >= 2.0f) {
            return 0.0f;
        }
        if (r < 1.0f) {
            return 3.0f * r * r2 - 5.0f * r2 + 2.0f;
        }
        return -r * r2 + 5.0f * r2 - 8.0f * r + 4.0f;
    }
    case Filter::Gaussian: {
        // scaled so that the edge of the width is two standard deviations out
        const float x = offset.x * 2.0f / width.x;
        const float y = offset.y * 2.0f / width.y;
        return std::exp(-2.0f * (x * x + y * y));
    }
    case Filter::Sinc: {
        const float pi = 3.14159265358979f;
        const float x = offset.x == 0.0f ? 1.0f : std::sin(pi * offset.x) / (pi * offset.x);
        const float y = offset.y == 0.0f ? 1.0f : std::sin(pi * offset.y) / (pi * offset.y);
        return x * y;
    }
    }
    return 0.0f;
}

bool resolution(float side) {
    return v3d::type::toCount(side, 1, largestResolution).has_value();
}

bool filterWidth(const glm::vec2 & width) {
    const bool wide = width.x > 0.0f && std::isfinite(width.x);
    const bool tall = width.y > 0.0f && std::isfinite(width.y);
    return wide && tall;
}

unsigned int sampleCount(float rate) {
    // the comparison is written so that a NaN takes one sample too
    if (!(rate >= 1.0f)) {
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
    return pinhole() ? 0.0f : focalLength / (2.0f * fstop);
}

bool Sampling::pinhole() const {
    return !(fstop > 0.0f) || std::isinf(fstop) || !(focalLength > 0.0f) || !(focalDistance > 0.0f);
}

};  // namespace v3d::render::offline
