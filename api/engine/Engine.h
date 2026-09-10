/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/config/Config.h>
#include <api/event/Engine.h>
#include <api/input/Engine.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Window.h>

#include <map>
#include <string>

#include "Accumulator.h"
#include "Statistics.h"

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
     * Offered every SDL event before the input devices see it.
     *
     * This is where an app puts a ui it did not write. A ui toolkit an app did not write
     * wants the events themselves rather than the commands the bindings turn them into,
     * and polling the keyboard instead is not the same thing: a press and a release inside
     * one frame poll as nothing having happened.
     *
     * Returning true consumes the event, so the input engine never maps it to a command -
     * a click that both presses a button and gives an order is what that prevents, and it
     * is the rule ui::Cursor::press() already applies inside api/ui. Per ADR-0043 the app
     * is asked first, and the engine's own handling of quit, resize and focus runs
     * whatever this returns.
     *
     * @return whether the app took the event
     **/
    virtual bool onEvent(const SDL_Event& event);

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
     * What the loop measured about its own pacing, per frame.
     **/
    const Statistics& statistics() const noexcept;

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
    Statistics statistics_;

    /**
     * Point a command at a different key than the config bound it to.
     *
     * The bindings are held as one mapper named "global" that has no way to be edited in
     * place, so this rebuilds it from the config document with the overrides applied.
     * That keeps one code path reading a binding rather than two that could disagree, and
     * it is why the override is remembered rather than written straight into the mapper:
     * the next rebind rebuilds from config again and would otherwise lose this one.
     *
     * Only the source's name changes. Whatever context and edge the config bound the
     * command under it keeps, so rebinding a key that fires on press does not silently
     * start firing on release too.
     *
     * What is not done here is remembering it across runs. A binding lives as long as the
     * process unless the app writes it somewhere, which engine::userPath() says where.
     *
     * @param command the destination the binding drives, as "context::name"
     * @param key the source event name to bind it to, which for a keyboard binding is a
     *        key name from api/input/Keyboard.cpp's table
     * @return whether the bindings were rebuilt
     **/
    bool rebind(const std::string& command, const std::string& key);

    /**
     * Offer one polled event to the app, the input devices and the engine, in that order.
     *
     * Separate from eventLoop() because that one renders and so cannot be driven in a
     * test, and the order the three are offered in is the part worth testing.
     **/
    void route(const SDL_Event& event);

 private:
     /**
      * Answer one event the input devices did not take - a quit, a resize, a focus
      * change. What the engine itself does with an event, as against when it looks for
      * one, which is eventLoop()'s.
      **/
     void handleEvent(const SDL_Event& event);

     bool registerEventMappings();

     /**
      * One end of a binding: the name and context it fires under, plus what that end
      * alone carries - the edge a source matches, and the parameter a destination
      * arrives with.
      * @return false when the mapping does not describe that end, which is logged
      **/
     bool readMappingSource(const boost::json::object& mapping, v3d::event::Event* event);
     bool readMappingDestination(const boost::json::object& mapping, v3d::event::Event* event);

     // destination identity -> the source name it should bind to instead of the
     // config's, applied every time the global mapper is rebuilt
     std::map<std::string, std::string> rebindings_;

     std::string appPath_;
     int features_;
     bool needShutdown_;
     bool quitting_;
     boost::shared_ptr<v3d::input::Engine> inputEngine_;
};

};  // namespace v3d::engine
