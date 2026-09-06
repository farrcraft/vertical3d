/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Image.h"

namespace v3d::image {

/**
 * What comparing two images found.
 *
 * The worst pixel is recorded whether or not the images matched, so a comparison that
 * passed still says how close it came. A pair that could not be compared at all - a
 * different size, a different format - has a reason instead and no worst pixel.
 **/
struct Difference final {
    bool match = true;
    unsigned int column = 0;
    unsigned int row = 0;
    unsigned int channel = 0;
    unsigned int delta = 0;
    unsigned int tolerance = 0;
    std::string reason;

    /**
     * One line saying what was wrong and where. A reference test that reports only
     * that two images differ costs an afternoon the first time it fires.
     **/
    std::string description() const;
};

/**
 * Compare two images channel by channel.
 *
 * The tolerance is there for float rounding across compilers, not for "close enough":
 * a test that needs a loose one is testing something it should not be.
 *
 * @param tolerance the largest per channel difference that still counts as a match
 **/
Difference compare(const Image & a, const Image & b, unsigned int tolerance);

};  // namespace v3d::image
