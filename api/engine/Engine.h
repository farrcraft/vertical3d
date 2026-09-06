/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Accumulator.h"

#include "../log/Logger.h"
#include "../asset/Manager.h"
#include "../config/Config.h"
#include "../input/Engine.h"
#include "../event/Engine.h"
#include "../render/realtime/Window.h"

#include <boost/json.hpp>
#include <entt/entt.hpp>

namespace v3d::engine {

/**
 * This is the game engine.
 * It is responsible for the main game loop
 **/
class Engine {
 public:
    /**
     * Constructor.
     *
     * @param appPath The fully qualified base path name from which all relative
     *                paths will be derived.
     **/
    explicit Engine(const std::string& appPath);

    /**
     * Initialize the engine.
     * Initialization includes only the minimal amount of work required to get
     * a window displayed on the screen.
     *
     * @param features The set of engine features to be enabled.
     * @return bool
     **/
    bool initialize(int features);

    /**
     * The game loop entry point
     * 
     * @return bool
     **/
    bool eventLoop();

    /**
     * Advance the game world time
     * @param delta milliseconds elapsed since the previous tick. Simulation that scales by
     *              this stays frame rate independent; simulation that ignores it does not.
     * @return bool
     **/
    virtual bool tick(unsigned int delta);

    /**
     * Advance the simulation by one fixed step.
     *
     * Called zero or more times per frame, however many whole steps the real time since the
     * last frame owes, per ADR-0032. Simulation belongs here and not in tick(): what runs
     * on a fixed step produces the same result whatever the frame rate was, and what runs
     * in tick() does not.
     *
     * @param step seconds of simulated time, always Accumulator::seconds
     * @return bool
     **/
    virtual bool simulate(float step);

    /**
     * The fraction of a simulation step elapsed but not yet simulated, in [0, 1).
     * A renderer that interpolates between the last two simulation states blends by this.
     **/
    float alpha() const noexcept;

    /**
     * Render the current frame.
     * This will be called after each tick within the event loop to draw the current frame
     * 
     * @return bool
     **/
    virtual bool render();

    /**
     * @return bool
     **/
    virtual bool shutdown();

    /**
     * Ask the game loop to stop after the frame it is on.
     *
     * This is what a quit command calls, and shutdown() is not: the loop ticks and
     * renders after an event handler returns, so tearing the window and SDL down from
     * inside a handler leaves the frame after it drawing against a destroyed window.
     * eventLoop() returns, and the caller shuts down once, outside the loop.
     **/
    void quit() noexcept;

    /**
     * @return whether something has asked the loop to stop
     **/
    bool quitting() const noexcept;

    /**
     * @return Window
     **/
    boost::shared_ptr<v3d::render::realtime::Window> window() const;

 protected:
    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<v3d::config::Config> config_;
    boost::shared_ptr<v3d::render::realtime::Window> window_;
    boost::shared_ptr<v3d::asset::Manager> assetManager_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    boost::shared_ptr<v3d::event::Engine> eventEngine_;
    entt::registry registry_;
    Accumulator accumulator_;

 private:
     bool registerEventMappings();

     /**
      * One end of a binding: the name and context it fires under, plus what that end
      * alone carries - the edge a source matches, and the parameter a destination
      * arrives with.
      * @return false when the mapping does not describe that end, which is logged
      **/
     bool readMappingSource(const boost::json::object& mapping, v3d::event::Event* event);
     bool readMappingDestination(const boost::json::object& mapping, v3d::event::Event* event);

     std::string appPath_;
     int features_;
     bool needShutdown_;
     bool quitting_;
     boost::shared_ptr<v3d::input::Engine> inputEngine_;
};

};  // namespace v3d::engine
