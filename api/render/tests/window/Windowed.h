/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/Engine3D.h>
#include <api/render/realtime/Window.h>

#include <boost/shared_ptr.hpp>

namespace v3d::test {

/**
 * What main() returns when no window can be made to present to. ctest is told to read it as a
 * skip rather than a failure.
 **/
const int skipExitCode = 77;

/**
 * A window and an Engine3D presenting to it, for a case that needs the whole frame loop.
 *
 * Each case gets its own, so a chain or a request one case leaves behind cannot reach another.
 * The window is small and is open only while the case runs.
 **/
struct Windowed {
    /**
     * @throw std::runtime_error when SDL cannot start, the window cannot be made, or no device
     *        can present to it
     **/
    Windowed();

    ~Windowed();

    Windowed(const Windowed&) = delete;
    Windowed& operator=(const Windowed&) = delete;

    /**
     * @return whether the validation layer was on and reported no errors
     **/
    bool silent() const;

    boost::shared_ptr<v3d::log::Logger> logger;
    boost::shared_ptr<render::realtime::Window> window;
    boost::shared_ptr<render::realtime::Engine3D> engine;
};

/**
 * @return whether a window can be made and presented to, so that a runner with no display can
 *         be told apart from a broken engine
 **/
bool windowAvailable();

};  // namespace v3d::test
