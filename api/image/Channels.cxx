/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Channels.h"

#include <cstddef>

namespace v3d::image {

void swapRedBlue(const unsigned char* from, unsigned char* to, std::size_t pixels, unsigned int channels) {
    for (std::size_t pixel = 0; pixel < pixels; ++pixel) {
        const unsigned char* source = from + pixel * channels;
        unsigned char* target = to + pixel * channels;
        // read both ends before writing either, so a swap in place does not read what it wrote
        const unsigned char first = source[0];
        const unsigned char third = source[2];
        target[0] = third;
        target[1] = source[1];
        target[2] = first;
        for (unsigned int channel = 3; channel < channels; ++channel) {
            target[channel] = source[channel];
        }
    }
}

};  // namespace v3d::image
