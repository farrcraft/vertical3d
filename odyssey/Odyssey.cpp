  // Vertical3D
  // Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)

#include <SDL.h>
#include <SDL_main.h>

#include "engine/Engine.h"

/**
 **/
int main(int argc, char* argv[]) {
    // extract exe path from argv (needed for loading file assets with relative paths)
    std::string appPath =
        boost::filesystem::path(boost::filesystem::system_complete(boost::filesystem::path(argv[0])).remove_filename()).string() +
        boost::filesystem::path("/").make_preferred().string();

    odyssey::engine::Engine engine(appPath);
    if (!engine.initialize()) {
        return EXIT_FAILURE;
    }

    const bool ok = engine.eventLoop();

    if (!engine.shutdown()) {
        return EXIT_FAILURE;
    }

    if (!ok) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
