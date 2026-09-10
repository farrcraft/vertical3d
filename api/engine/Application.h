/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <cstdlib>
#include <exception>
#include <string>
#include <utility>

namespace v3d::engine {

/**
 * The directory the running executable sits in, with a trailing separator.
 *
 * This is what an engine resolves its relative asset paths against, and argv[0] is the
 * only thing that knows it - a game is as likely to be started from another directory as
 * from its own.
 *
 * @param executable argv[0]
 **/
std::string appPath(const char* executable);

/**
 * The directory this user's own files for an app belong in, with a trailing separator.
 *
 * The symmetric question to the one appPath() answers. That one says where an app reads
 * the assets it shipped with; this one says where it writes what the player chose - a
 * settings document, a key binding, a saved game. They are different directories because
 * the first is overwritten from source on every build and is not reliably writable at all.
 *
 * Nothing else is needed to read or write there: asset::Manager takes its root as a
 * constructor argument, so a second manager on this path loads through the same loaders.
 *
 * A platform that cannot answer gives an empty string and a log line rather than throwing,
 * because a game that cannot find a settings directory should still run on its defaults.
 * The directory is created if it does not exist.
 *
 * @param org the organization the app belongs to, the same for every app that shares it
 * @param app what this app is called, and never changed once it has been chosen
 * @return the directory, ending in a separator, or an empty string
 **/
std::string userPath(const std::string& org, const std::string& app);

/**
 * Build an engine, run it to completion and shut it down - the whole of an app's main.
 *
 * shutdown() runs outside the loop and outside the catch, because it has to run whether
 * the loop ended by being asked to or by throwing, and because quit() is what an event
 * handler calls: tearing the window down from inside one leaves the frame after it drawing
 * against a destroyed window.
 *
 * @param T the engine to run, which is v3d::engine::Engine subclassed by the app. It has
 *          to carry an initialize() of its own taking no arguments, which is where the app
 *          names the features it wants
 * @param executable argv[0]
 * @param name what the app is called, for the one line a failure is reported on
 * @param args whatever else the app's engine is built from, forwarded after the path. An
 *        app that parses its command line into options before the engine exists has
 *        nowhere else to hand them over, and writing its own main to do it means writing
 *        this function's ordering out a second time
 * @return the process exit status
 **/
template <typename T, typename... Args>
int run(const char* executable, const std::string& name, Args&&... args) {
    T engine(appPath(executable), std::forward<Args>(args)...);

    // the renderer reports what it cannot do by throwing, and an uncaught exception on
    // windows is an abort dialog with no message in it. A windowed app has no console,
    // so the log is the only place what went wrong is readable
    int exitStatus = EXIT_SUCCESS;
    try {
        if (!engine.initialize() || !engine.eventLoop()) {
            exitStatus = EXIT_FAILURE;
        }
    } catch (const std::exception& error) {
        v3d::log::Logger logger;
        logger.get()->error("{} failed: {}", name, error.what());
        exitStatus = EXIT_FAILURE;
    }

    if (!engine.shutdown()) {
        exitStatus = EXIT_FAILURE;
    }

    return exitStatus;
}

};  // namespace v3d::engine
