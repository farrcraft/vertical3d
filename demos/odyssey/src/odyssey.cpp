/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include <api/engine/Application.h>
#include <api/engine/Main.h>
#include <demos/odyssey/src/engine/Engine.h>

int main(int /* argc */, char* argv[]) {
    return v3d::engine::run<odyssey::engine::Engine>(argv[0], { .organization = "Vertical3D", .application = "Odyssey" });
}
