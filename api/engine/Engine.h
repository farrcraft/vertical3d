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
     * Not virtual - ADR-0080. The order is the engine's, and an app supplies what runs at its
     * end rather than wrapping the whole and calling back in.
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
     * Ask the game loop to stop after the frame it is on.
     *
     * This is what a quit command calls, and shutdown() is not: the loop ticks and
     * renders after an event handler returns, so tearing the window and SDL down from
     * inside a handler leaves the frame after it drawing against a destroyed window.
     * eventLoop() returns, and run() shuts down once, outside the loop. shutdown() is not
     * reachable from an app at all - ADR-0080.
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
     * clears the edges after render(), so a tick or a simulate step sees the frame it is
     * part of and never the one before.
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
     * Asked of what the keyboard holds rather than of the edges the command was sent, so it
     * answers the same for a binding that fires on press alone, follows a rebind with
     * nothing more, and lets go when the keyboard does. Only keys are asked - a mouse button
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
     * What the engine sets up before start(). Every app in this tree wants all four, so
     * that is the default and only an app that wants fewer says so.
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
     * none of that type - which is a document an app treats as optional.
     **/
    const boost::json::object* document(v3d::config::Type type) const;

    /**
     * The same for a type the app names itself - one the api has no enum for.
     **/
    const boost::json::object* document(std::string_view type) const;

    /**
     * Time what happens until the scope ends, as a span the statistics report by name - the
     * one thing an app writes into what the loop measures.
     **/
    Statistics::Scope measure(std::string_view name);

    /**
     * The one registry an app's entities live in. Protected and writable on purpose: an app
     * is its entities, and every system it runs reaches them here.
     **/
    entt::registry registry_;

    /**
     * Point a command at a different key than the config bound it to - event::Bindings says
     * how, and keeps the context and the edge the config gave it.
     *
     * What is not done here is remembering it across runs. A binding lives as long as the
     * process unless the app writes it somewhere, which engine::userPath() says where.
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

     /**
      * Answer the one command every app means the same thing by: "ui::quit", which a menu's
      * quit item and a quit key both send, ends the loop as a closed window does.
      **/
     void command(const v3d::event::Event& event);

     /**
      * The three parts of initialize() that a feature turns on: the config and the bindings
      * it names, the input devices, and the window the window config sizes. Each false is a
      * startup that cannot go on, and has said why.
      **/
     bool loadConfig();
     void startInput();
     bool openWindow();

     // what the binding config says, which held() asks and rebind() rebuilds
     boost::shared_ptr<v3d::event::Bindings> bindings_;

     /**
      * The app's release(), then the window and SDL. Private, and reached only through
      * run(), so no event handler can tear the window down under the frame after it -
      * ADR-0080.
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
