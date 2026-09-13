/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Crop.h"

namespace v3d::image {

/**
 **/
boost::shared_ptr<Image> crop(const Image & source, unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
    boost::shared_ptr<Image> cut;

    const unsigned int depth = source.bpp() / 8;
    // the bounds are written as subtractions from the source rather than additions to the
    // offset, so that an x or a width near the top of the range cannot wrap into looking
    // like it fits
    if (depth == 0 || width == 0 || height == 0 ||
        x > source.width() || width > source.width() - x ||
        y > source.height() || height > source.height() - y) {
        return cut;
    }

    cut.reset(new Image(width, height, source.bpp()));
    const unsigned int run = width * depth;
    for (unsigned int row = 0; row < height; ++row) {
        const unsigned int from = ((y + row) * source.width() + x) * depth;
        const unsigned int to = row * run;
        for (unsigned int byte = 0; byte < run; ++byte) {
            (*cut)[to + byte] = source[from + byte];
        }
    }
    return cut;
}

};  // namespace v3d::image
