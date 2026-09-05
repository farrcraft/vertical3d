/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Controller.h"

// the WinMain a windows subsystem executable is entered through, which calls this main
#include <SDL3/SDL_main.h>

#include <cstdlib>
#include <exception>
#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "../../api/log/Logger.h"

int main(int /* argc */, char *argv[]) {
    // extract exe path from argv (needed for loading file assets with relative paths)
    std::string appPath =
        boost::filesystem::path(boost::filesystem::system_complete(boost::filesystem::path(argv[0])).remove_filename()).string() +
        boost::filesystem::path("/").make_preferred().string();

    Controller controller(appPath);

    // the renderer reports what it cannot do by throwing, and an uncaught exception on
    // windows is an abort dialog with no message in it. A windowed app has no console,
    // so the log is the only place what went wrong is readable
    int exitStatus = EXIT_SUCCESS;
    try {
        if (!controller.initialize() || !controller.eventLoop()) {
            exitStatus = EXIT_FAILURE;
        }
    } catch (const std::exception& error) {
        v3d::log::Logger logger;
        logger.get()->error("tetris failed: {}", error.what());
        exitStatus = EXIT_FAILURE;
    }

    if (!controller.shutdown()) {
        exitStatus = EXIT_FAILURE;
    }

    return exitStatus;
}
