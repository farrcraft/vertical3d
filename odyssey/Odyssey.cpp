/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Odyssey.h"

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
        return -1;
    }

    const bool ok = engine.run();

    if (!engine.shutdown()) {
        return -1;
    }

    if (!ok) {
        return -1;
    }

    return 0;
}
