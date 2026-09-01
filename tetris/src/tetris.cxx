/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Controller.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

int main(int argc, char *argv[]) {
    // extract exe path from argv (needed for loading file assets with relative paths)
    std::string appPath =
        boost::filesystem::path(boost::filesystem::system_complete(boost::filesystem::path(argv[0])).remove_filename()).string() +
        boost::filesystem::path("/").make_preferred().string();

    Controller controller(appPath);

    // the renderer reports what it cannot do by throwing, and an uncaught exception on
    // windows is an abort dialog with no message in it
    int exitStatus = EXIT_SUCCESS;
    try {
        if (!controller.initialize() || !controller.eventLoop()) {
            exitStatus = EXIT_FAILURE;
        }
    } catch (const std::exception& error) {
        std::cerr << "tetris failed: " << error.what() << std::endl;
        exitStatus = EXIT_FAILURE;
    }

    if (!controller.shutdown()) {
        exitStatus = EXIT_FAILURE;
    }

    return exitStatus;
}
