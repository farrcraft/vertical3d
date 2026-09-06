/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>

#include <cstdlib>
#include <exception>
#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "AppEngine.h"

int main(int /* argc */, char* argv[]) {
    // every asset resolves relative to the executable, so that is the path the engine is given
    const std::string appPath =
        boost::filesystem::path(boost::filesystem::system_complete(boost::filesystem::path(argv[0])).remove_filename()).string() +
        boost::filesystem::path("/").make_preferred().string();

    AppEngine engine(appPath);

    // the renderer reports what it cannot do by throwing, and the log is where an app with no
    // console can say so
    int exitStatus = EXIT_SUCCESS;
    try {
        if (!engine.initialize() || !engine.eventLoop()) {
            exitStatus = EXIT_FAILURE;
        }
    } catch (const std::exception& error) {
        v3d::log::Logger logger;
        logger.get()->error("starter failed: {}", error.what());
        exitStatus = EXIT_FAILURE;
    }

    if (!engine.shutdown()) {
        exitStatus = EXIT_FAILURE;
    }

    return exitStatus;
}
