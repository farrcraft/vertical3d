/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Sampling.h"

#include <api/type/Checked.h>

#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

namespace v3d::rib {

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

};  // namespace v3d::rib
