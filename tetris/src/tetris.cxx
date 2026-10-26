/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/Application.h>
#include <api/engine/Main.h>

#include "Controller.h"

int main(int /* argc */, char *argv[]) {
    return v3d::engine::run<Controller>(argv[0], { .organization = "Vertical3D", .application = "Tetris" });
}
