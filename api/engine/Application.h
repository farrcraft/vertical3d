/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdlib>
#include <exception>
#include <string>

#include "../log/Logger.h"

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
 * @return the process exit status
 **/
template <typename T>
int run(const char* executable, const std::string& name) {
    T engine(appPath(executable));

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
