/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "Controller.h"


int main(int argc, char *argv[]) {
    // extract exe path from argv (needed for loading file assets with relative paths)
    std::string appPath =
        boost::filesystem::path(boost::filesystem::system_complete(boost::filesystem::path(argv[0])).remove_filename()).string() +
        boost::filesystem::path("/").make_preferred().string();

    try {
        v3d::editor::Controller controller(appPath);
        if (!controller.initialize()) {
            return EXIT_FAILURE;
        }

        const bool ok = controller.eventLoop();

        if (!controller.shutdown()) {
            return EXIT_FAILURE;
        }
        if (!ok) {
            return EXIT_FAILURE;
        }
    } catch (const std::exception& e) {
        // an uncaught exception aborts into a message-less dialog, which says nothing about
        // what went wrong and leaves the process alive
        std::cerr << "Vertical|3D failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
