/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/engine/Identity.h>
#include <api/engine/Launch.h>
#include <api/log/Logger.h>

#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace v3d::engine {

/**
 * The directory the running executable sits in, with a trailing separator.
 *
 * An engine resolves its relative asset paths against this. The platform reports it, so it does
 * not depend on the working directory, or on whether the app was started from a shortcut or
 * from PATH. It is UTF-8.
 *
 * When the platform cannot say, this logs the failure and returns an empty string, so paths
 * resolve against the working directory.
 **/
std::string appPath();

/**
 * The directories an app's assets are read from, searched in order.
 *
 * The app's own assets are in data/<program>/, and the assets every app shares are in data/.
 * The build and the install lay them out the same way, so this is the one place the layout is
 * written down.
 *
 * @param appPath the directory the executable is in, ending in a separator
 * @param launch the command line, whose program names the app's own directory. An engine with
 *        no command line has no program, and reads data/ alone
 * @return the directories, each ending in a separator
 **/
std::vector<std::string> dataRoots(const std::string& appPath, const Launch& launch);

/**
 * The directory this user's own files for an app belong in, with a trailing separator.
 *
 * appPath() is where an app reads the assets it shipped with. This is where it writes what
 * the player chose: a settings document, a key binding, a saved game. The two differ because
 * appPath() is overwritten on every build and may not be writable at all.
 *
 * asset::Manager takes its root directory as a constructor argument. A second manager built on
 * this path, with the same loaders registered, reads and writes there.
 *
 * When the platform cannot provide the directory, this logs the failure and returns an empty
 * string. It does not throw, so a game without a settings directory still runs on its
 * defaults. The directory is created if it does not exist.
 *
 * @param org the organization the app belongs to, the same for every app that shares it
 * @param app what this app is called, and never changed once it has been chosen
 * @return the directory, ending in a separator, or an empty string
 **/
std::string userPath(const std::string& org, const std::string& app);

/**
 * The file an app's log is written to.
 *
 * It is v3d.log in userPath(), because an installed app's own directory may not be writable. An
 * app with no identity, or on a platform with no user directory, writes it beside the
 * executable. Logger::open() sends the log to stderr when the file cannot be written.
 *
 * @param identity who the app is
 * @return the log file's path
 **/
std::string logPath(const Identity& identity);

/**
 * The directory the documents a user makes with an app belong in, with a trailing separator.
 *
 * It is a directory named for the app's organization, in the user's documents folder, so the
 * user finds their work where they keep the rest of it. It is created if it does not exist. On
 * a platform with no documents folder it is userPath().
 *
 * @param identity who the app is
 * @return the directory, ending in a separator, or an empty string for an app with no identity,
 *         or when the platform has neither directory
 **/
std::string documentsPath(const Identity& identity);

/**
 * The body both forms of run() share. An app calls run().
 *
 * @tparam T the engine to run
 * @param launch what the command line asked of the engine
 * @param identity who the app is
 * @param args further constructor arguments for the app's engine, forwarded after the path
 * @return the process exit status
 **/
template <typename T, typename... Args>
int runLaunched(const Launch& launch, const Identity& identity, Args&&... args) {
    const std::string path = appPath();
    // a windowed app has no console, so the log is the only place its errors appear
    v3d::log::Logger::open(logPath(identity));
    T engine(path, std::forward<Args>(args)...);
    engine.identity(identity);
    engine.launch(launch);
    const std::string& name = identity.application;

    // the renderer reports failures by throwing, and an uncaught exception on Windows shows
    // an abort dialog with no message, so the exception is caught and logged here
    int exitStatus = EXIT_SUCCESS;
    try {
        if (!engine.initialize() || !engine.eventLoop()) {
            exitStatus = EXIT_FAILURE;
        }
    } catch (const std::exception& error) {
        const v3d::log::Logger logger;
        logger.get()->error("{} failed: {}", name, error.what());
        exitStatus = EXIT_FAILURE;
    } catch (...) {
        const v3d::log::Logger logger;
        logger.get()->error("{} failed with something that is not a std::exception", name);
        exitStatus = EXIT_FAILURE;
    }

    // a throw from release(), such as a lost device found while waiting for it to go idle,
    // is caught and logged in the same way. The window is then destroyed by ~Engine, after the
    // app's own members
    try {
        if (!engine.shutdown()) {
            exitStatus = EXIT_FAILURE;
        }
    } catch (const std::exception& error) {
        const v3d::log::Logger logger;
        logger.get()->error("{} failed to shut down: {}", name, error.what());
        exitStatus = EXIT_FAILURE;
    } catch (...) {
        const v3d::log::Logger logger;
        logger.get()->error("{} failed to shut down with something that is not a std::exception", name);
        exitStatus = EXIT_FAILURE;
    }

    return exitStatus;
}

/**
 * Build an engine, run it to completion and shut it down. This is all of an app's main().
 *
 * shutdown() runs outside the loop and after its catch, so it runs whether the loop ended
 * normally or by throwing. A throw from shutdown() is caught and logged too. An event handler
 * calls quit() instead: tearing the window down inside a handler would leave the next frame
 * drawing to a destroyed window.
 *
 * @tparam T the engine to run: the app's subclass of v3d::engine::Engine, with its own
 *           features(), start() and release(). This function is the only caller of
 *           shutdown(), which an app cannot reach
 * @param executable argv[0], which names the app's own data directory
 * @param identity who the app is. The engine is given it before it starts, and keeps the
 *        player's settings under it. Its application names the app in the log line that
 *        reports a failure
 * @param args further constructor arguments for the app's engine, forwarded after the path.
 *        An app that parses its command line into options passes them here rather than
 *        writing its own main
 * @return the process exit status
 **/
template <typename T, typename... Args>
int run(const char* executable, const Identity& identity, Args&&... args) {
    const char* const argv[] = { executable };
    return runLaunched<T>(Launch::parse(1, argv), identity, std::forward<Args>(args)...);
}

/**
 * Run an engine as run(executable, identity) does, with what its command line asks of it.
 *
 * The command line is read into a Launch, which the engine is given before it starts. A command
 * line Launch::parse() refuses is logged, and the app does not start.
 *
 * @tparam T the engine to run
 * @param argc the number of arguments, as main() is given it
 * @param argv the arguments, as main() is given them
 * @param identity who the app is
 * @param args further constructor arguments for the app's engine, forwarded after the path
 * @return the process exit status
 **/
template <typename T, typename... Args>
int run(int argc, char* argv[], const Identity& identity, Args&&... args) {
    Launch launch;
    try {
        launch = Launch::parse(argc, argv);
    } catch (const std::invalid_argument& error) {
        v3d::log::Logger::open(logPath(identity));
        const v3d::log::Logger logger;
        logger.get()->error("{} cannot start: {}", identity.application, error.what());
        return EXIT_FAILURE;
    }
    return runLaunched<T>(launch, identity, std::forward<Args>(args)...);
}

};  // namespace v3d::engine
