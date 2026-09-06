/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/Application.h>

#include "AppEngine.h"

int main(int /* argc */, char* argv[]) {
    // run() is the whole of a main, per ADR-0028: it derives the path every asset resolves
    // against from argv[0], drives initialize and the loop inside a try that logs what a
    // renderer threw, and shuts down outside it
    return v3d::engine::run<AppEngine>(argv[0], "starter");
}
