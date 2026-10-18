/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Compare.h"

#include <cstdlib>
#include <string>

namespace v3d::image {

std::string Difference::description() const {
    if (!reason.empty()) {
        return reason;
    }
    const std::string where = "worst pixel at column " + std::to_string(column) +
        ", row " + std::to_string(row) +
        ", channel " + std::to_string(channel) +
        " differs by " + std::to_string(delta) +
        " against a tolerance of " + std::to_string(tolerance);
    return match ? "images match - " + where : "images differ - " + where;
}

Difference compare(const Image & expected, const Image & actual, unsigned int tolerance) {
    Difference difference;
    difference.tolerance = tolerance;

    if (expected.width() != actual.width() || expected.height() != actual.height()) {
        difference.match = false;
        difference.reason = "dimensions differ - " +
            std::to_string(expected.width()) + "x" + std::to_string(expected.height()) + " against " +
            std::to_string(actual.width()) + "x" + std::to_string(actual.height());
        return difference;
    }
    if (expected.bpp() != actual.bpp()) {
        difference.match = false;
        difference.reason = "formats differ - " +
            std::to_string(expected.bpp()) + " bits per pixel against " + std::to_string(actual.bpp());
        return difference;
    }

    const unsigned int channels = expected.bpp() / 8;
    for (unsigned int row = 0; row < expected.height(); row++) {
        for (unsigned int column = 0; column < expected.width(); column++) {
            const unsigned int index = ((row * expected.width()) + column) * channels;
            for (unsigned int channel = 0; channel < channels; channel++) {
                const int delta = std::abs(static_cast<int>(expected[index + channel]) - static_cast<int>(actual[index + channel]));
                if (static_cast<unsigned int>(delta) > difference.delta) {
                    difference.delta = static_cast<unsigned int>(delta);
                    difference.column = column;
                    difference.row = row;
                    difference.channel = channel;
                }
            }
        }
    }

    difference.match = difference.delta <= tolerance;
    return difference;
}

};  // namespace v3d::image
