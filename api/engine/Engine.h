/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/config/Config.h>
#include <api/event/Bindings.h>
#include <api/event/Commands.h>
#include <api/event/Engine.h>
#include <api/input/Engine.h>
#include <api/log/Logger.h>
#include <api/window/Window.h>

#include <map>
#include <string>
#include <string_view>
#include <vector>

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
     * Destroys the window and quits SDL if shutdown() has not. That happens in a test, or in a
     * run that threw before shutdown(). It does not call the app's release(): when a base
     * class destructor runs, the app's own members are already destroyed.
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
     * The game loop: poll events, tick, simulate each fixed step that is due, and render,
     * until quit() is called.
     *
     * While the window is minimized the loop sleeps until the next event, and neither ticks,
     * simulates nor renders. A minimized window shows nothing, so a game pauses there and
     * resumes where it stopped. The time spent minimized is not counted as a frame.
     *
     * @return false when tick(), simulate() or render() returned false
     **/
    bool eventLoop();

    /**
     * Offered every SDL event before the input devices see it.
     *
     * An app feeds a third-party ui toolkit from here. Such a toolkit needs the raw events,
     * not the commands the bindings make from them. Polling the keyboard does not replace
     * this, because a press and a release inside one frame poll as no change.
     *
     * Returning true consumes the event, and the input engine never maps it to a command.
     * One click then cannot both press a button and issue an order. ui::Cursor::press()
     * applies the same rule inside api/ui. The engine handles quit, resize and focus events
     * whatever this returns.
     *
     * @return whether the app took the event
     **/
    virtual bool onEvent(const SDL_Event& event);

    /**
     * Called once per frame, before the frame's simulate() steps. Simulation goes in
     * simulate(), not here.
     *
     * @param delta milliseconds elapsed since the previous tick
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
     * Render the current frame. Called once per frame, after tick() and the frame's
     * simulate() steps.
     *
     * @return bool
     **/
    virtual bool render();

    /**
     * Stop the game loop, without asking the app. The ui::quit command and a closed window go
     * through quitRequested() first, and an app that held a quit there calls this once it may
     * end.
     *
     * Called from an event handler, it stops the loop before that frame's tick() and
     * render(), and the remaining events are not routed. Called from tick(), simulate() or
     * render(), it stops the loop when the frame ends. eventLoop() then returns, and run()
     * shuts down once, outside the loop. An app cannot call shutdown().
     **/
    void quit() noexcept;

    /**
     * @return whether quit() has been called
     **/
    bool quitting() const noexcept;

    /**
     * @return Window
     **/
    boost::shared_ptr<v3d::window::Window> window() const;

    /**
     * What the engine built, for an app to build on. Each is null until initialize() runs.
     * config() stays null unless the app asks for Feature::Config.
     **/
    const boost::shared_ptr<v3d::log::Logger>& logger() const noexcept;
    const boost::shared_ptr<v3d::config::Config>& config() const noexcept;
    const boost::shared_ptr<v3d::asset::Manager>& assets() const noexcept;
    const boost::shared_ptr<entt::dispatcher>& dispatcher() const noexcept;
    const boost::shared_ptr<v3d::event::Engine>& events() const noexcept;

    /**
     * The keys held now, and the edges: the keys pressed or released during this frame's
     * events. The loop clears the edges after render(), so tick() sees only the edges of its
     * own frame.
     *
     * Read the edges in tick() or render(), which run once a frame. A frame runs one
     * simulate() step for each step of elapsed time, which can be none or several. An edge
     * read in simulate() can therefore be missed or seen twice. Read held() there instead.
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
     * Reads the keyboard's current state, not the edges the command was sent on. The result
     * is the same for a binding that fires only on press. It follows a rebind, and it ends
     * when the key is released. Only keys count: a mouse button bound to a command never
     * holds it.
     *
     * A command is its name and context. A binding's param is not part of it. Two commands
     * that have to be held separately are bound under different names.
     *
     * @param command the destination as "context::name", as rebind() takes it
     * @return false when nothing is bound to it, or without Feature::KeyboardInput
     **/
    bool held(std::string_view command) const;

    /**
     * Every command the app handles, by "context::name". The engine handles a command event
     * only by running the handler registered here under its name.
     *
     * An app registers its commands in its constructor, including the ones it reads with
     * held(), so that a test can build the app without a window and check its bindings. A
     * handler runs only once the loop has started, so it may use what start() builds. The
     * engine registers "ui::quit" and "ui::fullscreen" itself.
     **/
    v3d::event::Commands& commands() noexcept;
    const v3d::event::Commands& commands() const noexcept;

    /**
     * The commands a binding sends that nothing has registered, in name order. initialize()
     * logs each one as an error once start() has run.
     *
     * @return none before the bindings are loaded, or without Feature::Config
     **/
    std::vector<std::string> unhandled() const;

    /**
     * The commands a binding document sends that nothing has registered, in name order. It
     * needs no window and no initialize(), so a test can build an app and check the bindings
     * it ships with.
     *
     * @param document a binding document, as data/mappings.json holds
     * @return the single entry "mappings" when the document cannot be read, and the failure is
     *         logged. A caller that expects an empty list then sees a failure.
     **/
    std::vector<std::string> unhandled(const boost::json::object& document) const;

 protected:
    /**
     * What the engine sets up before start(). Every feature by default. An app that needs
     * fewer overrides this.
     **/
    virtual Features features() const;

    /**
     * The app's own startup. It runs after every feature is set up: the window is open, the
     * config is read and the bindings are built.
     *
     * @return false to stop startup, which run() reports as a failed run
     **/
    virtual bool start();

    /**
     * The app's own teardown. It runs before the engine destroys the window. Anything that
     * presents to the window, a renderer above all, is released here, while the window still
     * exists to wait for the device to go idle. Only the engine calls this.
     *
     * @return false when something failed to release, which run() reports
     **/
    virtual bool release();

    /**
     * The user asked to quit, with the ui::quit command or by closing the window. Only the
     * engine calls this.
     *
     * An app with work it could lose overrides it to ask first. It returns false to hold the
     * quit, and calls quit() itself once the user has answered.
     *
     * @return true to quit now, which the default always does
     **/
    virtual bool quitRequested();

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
     * The registry that holds all of an app's entities. Writable, so every system the app
     * runs can reach it.
     **/
    entt::registry& registry() noexcept;
    const entt::registry& registry() const noexcept;

    /**
     * Bind a command to a different key than the config gave it, keeping the context and
     * the edge from the config.
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
     * The bindings the binding config installed, for an app's controls screen: what a command
     * is bound to now, what a key already fires, a rebind of one key of several, a reset, and
     * the overrides to save and restore.
     *
     * @return null when the app has no binding config
     **/
    boost::shared_ptr<v3d::event::Bindings> bindings() const noexcept;

    /**
     * Offer one polled event to the app, the input devices and the engine, in that order.
     *
     * eventLoop() renders, and cannot run in a test. This is a separate function so that a
     * test can check the order.
     **/
    void route(const SDL_Event& event);

 private:
     /**
      * Handle one event the input devices did not take: a quit, a resize, a focus change or
      * a window minimized or restored.
      **/
     void handleEvent(const SDL_Event& event);

     /**
      * Quit, if the app does not hold the quit. Every request from the user comes here.
      **/
     void requestQuit();

     /**
      * Run the handler a command event names. An event nothing handles is logged.
      **/
     void command(const v3d::event::Event& event);

     /**
      * The three parts of initialize() that a feature turns on: the config and the bindings
      * it names, the input devices, and the window the window config sizes. loadConfig() and
      * openWindow() log the reason and return false when startup cannot continue.
      **/
     bool loadConfig();
     void startInput();
     bool openWindow();

     entt::registry registry_;

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
     boost::shared_ptr<v3d::window::Window> window_;
     boost::shared_ptr<v3d::asset::Manager> assetManager_;
     boost::shared_ptr<entt::dispatcher> dispatcher_;
     boost::shared_ptr<v3d::event::Engine> eventEngine_;
     Accumulator accumulator_;
     Statistics statistics_;

     std::string appPath_;
     Features features_;
     bool needShutdown_;
     bool released_ = false;
     // declared after dispatcher_, so it disconnects before the dispatcher is destroyed
     entt::scoped_connection commandListener_;
     v3d::event::Commands commands_;
     bool quitting_;
     bool minimized_ = false;
     boost::shared_ptr<v3d::input::Engine> inputEngine_;
};

};  // namespace v3d::engine
