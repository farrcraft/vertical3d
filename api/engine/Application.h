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
 * An engine resolves its relative asset paths against this. It comes from argv[0] because a
 * game can be started from any working directory.
 *
 * @param executable argv[0]
 **/
std::string appPath(const char* executable);

/**
 * The directory this user's own files for an app belong in, with a trailing separator.
 *
 * appPath() is where an app reads the assets it shipped with. This is where it writes what
 * the player chose: a settings document, a key binding, a saved game. The two differ because
 * appPath() is overwritten on every build and may not be writable at all.
 *
 * Nothing else is needed to read or write there: asset::Manager takes its root as a
 * constructor argument, so a second manager on this path loads through the same loaders.
 *
 * A platform that cannot provide one gives an empty string and a log line rather than
 * throwing, because a game without a settings directory should still run on its defaults.
 * The directory is created if it does not exist.
 *
 * @param org the organization the app belongs to, the same for every app that shares it
 * @param app what this app is called, and never changed once it has been chosen
 * @return the directory, ending in a separator, or an empty string
 **/
std::string userPath(const std::string& org, const std::string& app);

/**
 * Build an engine, run it to completion and shut it down. This is all of an app's main().
 *
 * shutdown() runs outside the loop and after its catch, so it runs whether the loop ended
 * normally or by throwing. A throw from shutdown() is caught and logged too. An event handler calls quit() instead: tearing the window down
 * inside a handler would leave the next frame drawing to a destroyed window.
 *
 * @param T the engine to run: the app's subclass of v3d::engine::Engine, with its own
 *          features(), start() and release(). This function is the only caller of
 *          shutdown(), which an app cannot reach
 * @param executable argv[0]
 * @param name what the app is called, used in the log line that reports a failure
 * @param args further constructor arguments for the app's engine, forwarded after the path.
 *        An app that parses its command line into options passes them here rather than
 *        writing its own main
 * @return the process exit status
 **/
template <typename T, typename... Args>
int run(const char* executable, const std::string& name, Args&&... args) {
    const std::string path = appPath(executable);
    // beside the executable, whatever directory it was started from: a windowed app has no
    // console, so the log is the only place its errors appear. A directory that cannot be
    // written to sends the log to stderr, and the app still runs
    v3d::log::Logger::open(path + "v3d.log");
    T engine(path, std::forward<Args>(args)...);

    // the renderer reports failures by throwing, and an uncaught exception on Windows shows
    // an abort dialog with no message, so the exception is caught and logged here
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

    // a throw from release(), such as a lost device found while waiting for it to go idle,
    // is caught and logged in the same way
    try {
        if (!engine.shutdown()) {
            exitStatus = EXIT_FAILURE;
        }
    } catch (const std::exception& error) {
        v3d::log::Logger logger;
        logger.get()->error("{} failed to shut down: {}", name, error.what());
        exitStatus = EXIT_FAILURE;
    }

    return exitStatus;
}

};  // namespace v3d::engine
