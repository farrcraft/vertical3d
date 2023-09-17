/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include <cstdlib>
#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "Controller.h"


int main(int argc, char *argv[]) {
    // extract exe path from argv (needed for loading file assets with relative paths)
    std::string appPath =
        boost::filesystem::path(boost::filesystem::system_complete(boost::filesystem::path(argv[0])).remove_filename()).string() +
        boost::filesystem::path("/").make_preferred().string();

    // seed the random number generator
    srand(static_cast<unsigned int>(time(nullptr)));

    Controller controller(appPath);
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

    return EXIT_SUCCESS;
}
