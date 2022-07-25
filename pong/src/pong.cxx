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
 * (*) fix menu system
 * (*) rendering code should go in PongRenderer class
 * ( ) support key rebinding
 * ( ) modify game vars
 * ( ) network multiplayer mode
 * ( ) p2p network mode
 * ( ) server network mode
 * ( ) save keybind / gamevar changes
 **/

#include <cstdlib>
#include <iostream>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "PongController.h"

int main(int argc, char *argv[]) {
    // extract exe path from argv (needed for loading file assets with relative paths)
    std::string appPath = 
        boost::filesystem::path(boost::filesystem::system_complete(boost::filesystem::path(argv[0])).remove_filename()).string() + 
        boost::filesystem::path("/").make_preferred().string();

    PongController controller(appPath);

    if (!controller.run()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

