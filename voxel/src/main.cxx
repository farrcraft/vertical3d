/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

// the WinMain a windows subsystem executable is entered through, which calls this main
#include <SDL3/SDL_main.h>

#include <cstdlib>
#include <ctime>

#include "Controller.h"

#include "../../api/engine/Application.h"


int main(int /* argc */, char *argv[]) {
    // seed the random number generator
    srand(static_cast<unsigned int>(time(nullptr)));

    return v3d::engine::run<Controller>(argv[0], "voxel");
}
