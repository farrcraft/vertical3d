/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Application.h"

#include <api/engine/Identity.h>
#include <api/engine/Launch.h>
#include <api/log/Logger.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>

#include <string>
#include <vector>

namespace v3d::engine {

/**
 **/
std::string appPath() {
    // SDL owns the string, ends it with a separator, and needs no SDL_Init()
    const char* path = SDL_GetBasePath();
    if (path == nullptr) {
        const v3d::log::Logger logger;
        logger.get()->error("the executable's directory is unknown - {}", SDL_GetError());
        return std::string();
    }
    return std::string(path);
}

/**
 **/
std::vector<std::string> dataRoots(const std::string& appPath, const Launch& launch) {
    const std::string shared = appPath + "data/";
    if (launch.program.empty()) {
        return { shared };
    }
    return { shared + launch.program + "/", shared };
}

/**
 **/
std::string logPath(const Identity& identity) {
    const std::string user = identity.named() ? userPath(identity.organization, identity.application) : std::string();
    return (user.empty() ? appPath() : user) + "v3d.log";
}

/**
 **/
std::string documentsPath(const Identity& identity) {
    if (!identity.named()) {
        return std::string();
    }
    const char* documents = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
    if (documents == nullptr) {
        return userPath(identity.organization, identity.application);
    }
    // SDL ends the folder with a separator
    std::string directory = std::string(documents) + identity.organization + "/";
    if (!SDL_CreateDirectory(directory.c_str())) {
        const v3d::log::Logger logger;
        logger.get()->error("cannot make {} - {}", directory, SDL_GetError());
        return userPath(identity.organization, identity.application);
    }
    return directory;
}

/**
 **/
std::string userPath(const std::string& org, const std::string& app) {
    // the caller frees the returned string with SDL_free(). SDL_GetPrefPath() needs no
    // SDL_Init(), so this can be called at the very start of an app's startup
    char* path = SDL_GetPrefPath(org.c_str(), app.c_str());
    if (path == nullptr) {
        const v3d::log::Logger logger;
        logger.get()->error("no user directory for {}/{} - {}", org, app, SDL_GetError());
        return std::string();
    }

    const std::string directory(path);
    SDL_free(path);
    // SDL guarantees the trailing separator, as appPath() does
    return directory;
}

};  // namespace v3d::engine
