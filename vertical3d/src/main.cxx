/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include <api/engine/Main.h>

#include "Controller.h"

int main(int /* argc */, char *argv[]) {
    return v3d::engine::run<v3d::editor::Controller>(argv[0], "Vertical|3D");
}
