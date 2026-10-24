/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Placed.h"

#include <vector>

namespace v3d::render::offline::sl {

namespace {

/** How many lights one light shines as. **/
unsigned int count(const Placed & light) {
    return light.area ? light.area->samples() : 1U;
}

};  // namespace

unsigned int shining(const std::vector<Placed> & lights) {
    unsigned int total = 0;
    for (const Placed & light : lights) {
        total += count(light);
    }
    return total;
}

const Placed* which(const std::vector<Placed> & lights, unsigned int index, unsigned int* sample) {
    unsigned int left = index;
    for (const Placed & light : lights) {
        if (left < count(light)) {
            *sample = left;
            return &light;
        }
        left -= count(light);
    }
    return nullptr;
}

};  // namespace v3d::render::offline::sl
