/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/config/Config.h>
#include <api/event/Bindings.h>
#include <api/event/Engine.h>
#include <api/input/Engine.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Window.h>

#include <map>
#include <string>
#include <string_view>

#include "Accumulator.h"
#include "Feature.h"
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
     * Releases the window and SDL if shutdown() never ran - a test, or a run that failed
     * before it got there. An app's own release() cannot run from here, because what it
     * releases has already been destroyed by the time a base destructor runs.
     **/
    virtual ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    /**
     * Start the engine: the features() the app asked for, then the app's own start().
     *
     * Not virtual: the engine controls the startup order, and an app adds its own work in
     * start().
     *
     * @return false when a feature or the app's start() failed, which is logged
     **/
    bool initialize();

    /**
     * The game loop entry point
     * 
     * @return bool
     **/
    bool eventLoop();

    /**
     * Offered every SDL event before the input devices see it.
     *
     * This is where an app feeds a third-party ui toolkit, which needs the raw events
     * rather than the commands the bindings make from them. Polling the keyboard is not a
     * substitute: a press and a release inside one frame poll as no change.
     *
     * Returning true consumes the event, so the input engine never maps it to a command.
     * One click then cannot both press a button and issue an order, the same rule
     * ui::Cursor::press() applies inside api/ui. The engine still handles quit, resize and
     * focus whatever this returns.
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
     * Called once for each whole fixed step in the real time since the last frame, so zero
     * or more times per frame. Simulation belongs here and not in tick(): a fixed step gives
     * the same result at any frame rate, and tick() does not.
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
     * Ask the game loop to stop after the frame it is on.
     *
     * A quit command calls this. The loop still ticks and renders after an event handler
     * returns, so the window cannot be torn down inside a handler. Instead eventLoop()
     * returns, and run() shuts down once, outside the loop. An app cannot call shutdown().
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

    /**
     * What the engine built, for an app to build on. Each is null until initialize() runs,
     * and the config until it runs with Feature::Config; the window needs Feature::Window.
     **/
    const boost::shared_ptr<v3d::log::Logger>& logger() const noexcept;
    const boost::shared_ptr<v3d::config::Config>& config() const noexcept;
    const boost::shared_ptr<v3d::asset::Manager>& assets() const noexcept;
    const boost::shared_ptr<entt::dispatcher>& dispatcher() const noexcept;
    const boost::shared_ptr<v3d::event::Engine>& events() const noexcept;

    /**
     * What the keyboard holds, and what changed edge during this frame's events. The loop
     * clears the edges after render(), so a tick sees the frame it is part of and never the
     * one before.
     *
     * Read the edges in tick() or render(), which run once a frame. A frame runs as many
     * simulate() steps as time has passed for, which can be none or several, so an edge read
     * in simulate() can be missed or seen twice. Read held() there instead.
     *
     * @return nullptr when the app did not ask for Feature::KeyboardInput
     **/
    const v3d::input::KeyState* keys() const;

    /**
     * The same for the mouse, plus where the cursor is.
     *
     * @return nullptr when the app did not ask for Feature::MouseInput
     **/
    const v3d::input::MouseState* mouse() const;

    /**
     * Whether a command is held: whether any key bound to it is down now.
     *
     * Reads the keyboard's current state rather than the edges the command was sent on. It
     * therefore gives the same result for a binding that fires on press alone, follows a
     * rebind automatically, and releases when the key does. Only keys count: a mouse button
     * bound to a command never holds it.
     *
     * A command is its name and context; a binding's param is not part of it, so commands
     * that are to be held apart are bound as commands of their own.
     *
     * @param command the destination as "context::name", as rebind() takes it
     * @return false when nothing is bound to it, or without Feature::KeyboardInput
     **/
    bool held(std::string_view command) const;

 protected:
    /**
     * What the engine sets up before start(). All four by default; an app that needs fewer
     * overrides this.
     **/
    virtual Features features() const;

    /**
     * The app's own startup, run once every feature is up - the window open, the config read,
     * the bindings built - so it can build on all of them.
     *
     * @return false to stop startup, which run() reports as a failed run
     **/
    virtual bool start();

    /**
     * The app's own teardown, run before the engine destroys the window: whatever presents
     * to the window, a renderer above all, has to let the device go idle while the window
     * still exists. The engine calls this; nothing else should.
     *
     * @return false when something failed to release, which run() reports
     **/
    virtual bool release();

    /**
     * One of the documents config.json names, or null when there is no config or it names
     * none of that type. An app treats every such document as optional.
     **/
    const boost::json::object* document(v3d::config::Type type) const;

    /**
     * The same for a type the app names itself - one the api has no enum for.
     **/
    const boost::json::object* document(std::string_view type) const;

    /**
     * Time the rest of the scope as a named span in the loop's statistics. This is how an
     * app adds its own timings.
     **/
    Statistics::Scope measure(std::string_view name);

    /**
     * The registry that holds all of an app's entities. Protected and writable on purpose,
     * so every system the app runs can reach it.
     **/
    entt::registry registry_;

    /**
     * Bind a command to a different key than the config gave it, keeping the context and
     * the edge from the config. event::Bindings does the work.
     *
     * The change is not saved. It lasts for the process unless the app stores it, for
     * example in a document under engine::userPath().
     *
     * @param command the destination the binding drives, as "context::name"
     * @param key the source event name to bind it to, which for a keyboard binding is a
     *        key name from api/input/Keyboard.cpp's table
     * @return whether the bindings were rebuilt, which is false with no binding config
     **/
    bool rebind(const std::string& command, const std::string& key);

    /**
     * Offer one polled event to the app, the input devices and the engine, in that order.
     *
     * Separate from eventLoop(), which renders and so cannot run in a test, so that the
     * order can be tested.
     **/
    void route(const SDL_Event& event);

 private:
     /**
      * Handle one event the input devices did not take: a quit, a resize or a focus
      * change. eventLoop() decides when to poll for events; this decides what the engine
      * does with one.
      **/
     void handleEvent(const SDL_Event& event);

     /**
      * Handle "ui::quit", which a menu's quit item and a quit key both send. It ends the
      * loop as a closed window does.
      **/
     void command(const v3d::event::Event& event);

     /**
      * The three parts of initialize() that a feature turns on: the config and the bindings
      * it names, the input devices, and the window the window config sizes. Each returns
      * false, after logging why, when startup cannot continue.
      **/
     bool loadConfig();
     void startInput();
     bool openWindow();

     // the bindings from the binding config, read by held() and rebuilt by rebind()
     boost::shared_ptr<v3d::event::Bindings> bindings_;

     /**
      * The app's release(), then the window and SDL. Private and called only by run(), so
      * no event handler can destroy the window while a frame still needs it.
      **/
     bool shutdown();

     template <typename T, typename... Args>
     friend int run(const char* executable, const std::string& name, Args&&... args);

     boost::shared_ptr<v3d::log::Logger> logger_;
     boost::shared_ptr<v3d::config::Config> config_;
     boost::shared_ptr<v3d::render::realtime::Window> window_;
     boost::shared_ptr<v3d::asset::Manager> assetManager_;
     boost::shared_ptr<entt::dispatcher> dispatcher_;
     boost::shared_ptr<v3d::event::Engine> eventEngine_;
     Accumulator accumulator_;
     Statistics statistics_;

     std::string appPath_;
     Features features_;
     bool needShutdown_;
     bool released_ = false;
     // after dispatcher_, so it disconnects before the dispatcher it points into can go
     entt::scoped_connection quitCommand_;
     bool quitting_;
     boost::shared_ptr<v3d::input::Engine> inputEngine_;
};

};  // namespace v3d::engine
