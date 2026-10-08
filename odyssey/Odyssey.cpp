/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include <api/engine/Main.h>
#include <odyssey/engine/Engine.h>

int main(int /* argc */, char* argv[]) {
    return v3d::engine::run<odyssey::engine::Engine>(argv[0], "odyssey");
}
