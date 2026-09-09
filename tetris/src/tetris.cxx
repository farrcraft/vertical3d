/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/Application.h>

#include "Controller.h"

// the WinMain a windows subsystem executable is entered through, which calls this main
#include <SDL3/SDL_main.h>

int main(int /* argc */, char *argv[]) {
    return v3d::engine::run<Controller>(argv[0], "tetris");
}
