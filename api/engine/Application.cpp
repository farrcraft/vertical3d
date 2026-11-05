/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Application.h"

#include <api/engine/Identity.h>
#include <api/engine/Launch.h>
#include <api/log/Logger.h>
#include <api/version/Version.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <cstdio>
#include <iostream>
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
void describe(const Identity& identity) {
    const std::string version(v3d::version::text());
    // SDL copies both strings. The identifier is left to SDL's default
    if (!SDL_SetAppMetadata(identity.application.c_str(), version.c_str(), nullptr)) {
        const v3d::log::Logger logger;
        logger.get()->warn("cannot name the app to SDL - {}", SDL_GetError());
    }
}

/**
 **/
void printVersion(const Identity& identity) {
#ifdef _WIN32
    // a windowed app has no standard output. Unless its output was redirected, it takes the
    // console it was started from, when there is one
    const void* const output = GetStdHandle(STD_OUTPUT_HANDLE);
    if ((output == nullptr || output == INVALID_HANDLE_VALUE) && AttachConsole(ATTACH_PARENT_PROCESS) != FALSE) {
        FILE* console = nullptr;
        if (freopen_s(&console, "CONOUT$", "w", stdout) != 0) {
            return;
        }
    }
#endif
    std::cout << identity.application << " " << v3d::version::text() << "\n";
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
