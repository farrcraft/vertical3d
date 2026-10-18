/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/Application.h>
#include <api/engine/Main.h>

#include <cstdlib>
#include <random>

#include "Controller.h"

int main(int /* argc */, char *argv[]) {
    // Chunk generation draws its block types from rand(). Seed it from the platform's entropy
    // source: two runs started in the same second would share a seed taken from the clock.
    std::random_device entropy;
    srand(entropy());

    return v3d::engine::run<Controller>(argv[0], "voxel");
}
