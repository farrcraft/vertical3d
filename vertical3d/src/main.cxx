/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

// the WinMain a windows subsystem executable is entered through, which calls this main
#include <SDL3/SDL_main.h>

#include <api/engine/Application.h>

#include "Controller.h"

int main(int /* argc */, char *argv[]) {
    return v3d::engine::run<v3d::editor::Controller>(argv[0], "Vertical|3D");
}
