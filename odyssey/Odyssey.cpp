/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

// the WinMain a windows subsystem executable is entered through, which calls this main
#include <SDL3/SDL_main.h>

#include "engine/Engine.h"

#include "../api/engine/Application.h"

int main(int /* argc */, char* argv[]) {
    return v3d::engine::run<odyssey::engine::Engine>(argv[0], "odyssey");
}
