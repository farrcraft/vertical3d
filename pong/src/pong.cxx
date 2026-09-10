/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

/**
 *
 * This version of pong began as a personal challenge to create the simple game.
 * Once a relatively complete and playable implementation was finished, much of
 * the common reusable code was extracted out into a separate library - libhookah.
 * Subsequent iterations of this source have been to support testing the evolving
 * design of that library. The core game logic has not otherwise significantly
 * changed since then.
 *
 *
 * TODO:
 * ( ) support key rebinding
 * ( ) modify game vars
 * ( ) network multiplayer mode
 * ( ) p2p network mode
 * ( ) server network mode
 * ( ) save keybind / gamevar changes
 **/

// the WinMain a windows subsystem executable is entered through, which calls this main

#include <api/engine/Application.h>

#include <SDL3/SDL_main.h>

#include "PongEngine.h"

int main(int /* argc */, char *argv[]) {
    return v3d::engine::run<PongEngine>(argv[0], "pong");
}
