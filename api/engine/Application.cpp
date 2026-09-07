/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Application.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>

#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace v3d::engine {

/**
 **/
std::string appPath(const char* executable) {
    const boost::filesystem::path directory =
        boost::filesystem::system_complete(boost::filesystem::path(executable)).remove_filename();
    return directory.string() + boost::filesystem::path("/").make_preferred().string();
}

/**
 **/
std::string userPath(const std::string& org, const std::string& app) {
    // SDL owns what it hands back and needs no SDL_Init to hand it back, so this is
    // callable from the earliest part of an app's startup
    char* path = SDL_GetPrefPath(org.c_str(), app.c_str());
    if (path == nullptr) {
        v3d::log::Logger logger;
        logger.get()->error("no user directory for {}/{} - {}", org, app, SDL_GetError());
        return std::string();
    }

    const std::string directory(path);
    SDL_free(path);
    // SDL guarantees the trailing separator, which is what appPath() also promises
    return directory;
}

};  // namespace v3d::engine
