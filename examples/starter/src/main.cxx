/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/Application.h>

#include "AppEngine.h"

int main(int /* argc */, char* argv[]) {
    // run() is all a main needs: the api supplies the startup, the loop and the shutdown,
    // and an app does not write its own
    return v3d::engine::run<AppEngine>(argv[0], "starter");
}
