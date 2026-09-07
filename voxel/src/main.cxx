/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

// the WinMain a windows subsystem executable is entered through, which calls this main
#include <SDL3/SDL_main.h>

#include <cstdlib>
#include <random>

#include "Controller.h"

#include "../../api/engine/Application.h"


int main(int /* argc */, char *argv[]) {
    // Chunk generation draws its block types from rand(). The seed comes from the platform's
    // entropy source rather than the clock, which two runs started in the same second share.
    std::random_device entropy;
    srand(entropy());

    return v3d::engine::run<Controller>(argv[0], "voxel");
}
