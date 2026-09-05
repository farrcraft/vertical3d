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
    std::string where = "worst pixel at column " + std::to_string(column) +
        ", row " + std::to_string(row) +
        ", channel " + std::to_string(channel) +
        " differs by " + std::to_string(delta) +
        " against a tolerance of " + std::to_string(tolerance);
    return match ? "images match - " + where : "images differ - " + where;
}

Difference compare(const Image & a, const Image & b, unsigned int tolerance) {
    Difference difference;
    difference.tolerance = tolerance;

    if (a.width() != b.width() || a.height() != b.height()) {
        difference.match = false;
        difference.reason = "dimensions differ - " +
            std::to_string(a.width()) + "x" + std::to_string(a.height()) + " against " +
            std::to_string(b.width()) + "x" + std::to_string(b.height());
        return difference;
    }
    if (a.bpp() != b.bpp()) {
        difference.match = false;
        difference.reason = "formats differ - " +
            std::to_string(a.bpp()) + " bits per pixel against " + std::to_string(b.bpp());
        return difference;
    }

    const unsigned int channels = a.bpp() / 8;
    for (unsigned int row = 0; row < a.height(); row++) {
        for (unsigned int column = 0; column < a.width(); column++) {
            unsigned int index = ((row * a.width()) + column) * channels;
            for (unsigned int channel = 0; channel < channels; channel++) {
                int delta = std::abs(static_cast<int>(a[index + channel]) - static_cast<int>(b[index + channel]));
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
