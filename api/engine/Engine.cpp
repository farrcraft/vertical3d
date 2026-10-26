/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

#include <api/asset/Manager.h>
#include <api/asset/Manifest.h>
#include <api/asset/kind/Json.h>
#include <api/asset/media/Loaders.h>
#include <api/event/Bindings.h>
#include <api/event/Router.h>
#include <api/event/Event.h>
#include <api/event/kind/WindowFocus.h>
#include <api/event/kind/WindowResize.h>
#include <api/input/DeviceType.h>
#include <api/input/Devices.h>
#include <api/input/KeyState.h>
#include <api/input/Keyboard.h>
#include <api/input/Mouse.h>
#include <api/input/MouseState.h>
#include <api/log/Logger.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Feature.h"
#include "Identity.h"
#include "Settings.h"
#include "Statistics.h"
#include "WindowSize.h"

#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include <boost/make_shared.hpp>

namespace v3d::engine {

namespace {

/** The settings key the player's key bindings are kept under. **/
const char* const BINDINGS = "bindings";

/**
 * The player's key bindings as the settings keep them: a list of overrides, each the command,
 * the source it replaces, and the name it binds instead.
 **/
boost::json::array save(const std::vector<v3d::event::Bindings::Override>& overrides) {
    boost::json::array saved;
    for (const v3d::event::Bindings::Override& made : overrides) {
        saved.push_back(boost::json::object{ { "command", made.command }, { "source", made.source }, { "name", made.name } });
    }
    return saved;
}

/**
 * The overrides a settings document holds. An entry of another shape, from a hand edit, is
 * left out.
 **/
std::vector<v3d::event::Bindings::Override> read(const boost::json::value& saved) {
    std::vector<v3d::event::Bindings::Override> overrides;
    if (!saved.is_array()) {
        return overrides;
    }
    for (const boost::json::value& entry : saved.as_array()) {
        const boost::json::object* fields = entry.if_object();
        if (fields == nullptr) {
            continue;
        }
        const boost::json::value* command = fields->if_contains("command");
        const boost::json::value* source = fields->if_contains("source");
        const boost::json::value* name = fields->if_contains("name");
        if (command == nullptr || !command->is_string() || source == nullptr || !source->is_string() ||
            name == nullptr || !name->is_string()) {
            continue;
        }
        overrides.push_back({ .command = std::string(command->as_string()), .source = std::string(source->as_string()),
            .name = std::string(name->as_string()) });
    }
    return overrides;
}

};  // namespace

/**
 **/
Engine::Engine(const std::string& appPath) :
    appPath_(appPath),
    needShutdown_(false),
    quitting_(false) {
    // the two commands every app shares. A menu's quit item and a quit key both send
    // "ui::quit", which ends the loop as a closed window does
    commands_.addPress("ui::quit", [this]() { requestQuit(); });
    commands_.addPress("ui::fullscreen", [this]() {
        if (window_) {
            window_->fullscreen(!window_->fullscreen());
        }
    });
}

/**
 **/
void Engine::quit() noexcept {
    quitting_ = true;
}

/**
 **/
bool Engine::quitRequested() {
    return true;
}

/**
 **/
void Engine::requestQuit() {
    if (quitRequested()) {
        quit();
    }
}

/**
 **/
bool Engine::quitting() const noexcept {
    return quitting_;
}

/**
 **/
void Engine::identity(const Identity& identity) {
    identity_ = identity;
}

/**
 **/
const Identity& Engine::identity() const noexcept {
    return identity_;
}

/**
 **/
const boost::shared_ptr<Settings>& Engine::settings() const noexcept {
    return settings_;
}

/**
 **/
void Engine::loadSettings() {
    if (!identity_.named()) {
        return;
    }
    settings_ = boost::make_shared<Settings>(identity_.organization, identity_.application, logger_);
    // a missing document is a player who changed nothing, and every setting takes its default
    settings_->load();
    if (!bindings_) {
        return;
    }
    const boost::json::value* saved = settings_->value(BINDINGS);
    if (saved != nullptr) {
        bindings_->restore(read(*saved));
    }
    // each change is saved as it is made. No exit path is certain to run, and a crash should
    // not lose a rebinding
    bindings_->changed([this]() {
        settings_->set(BINDINGS, boost::json::value(save(bindings_->overrides())));
        settings_->save();
    });
}

/**
 **/
bool Engine::rebind(const std::string& command, const std::string& key) {
    return bindings_ && bindings_->rebind(command, key);
}

boost::shared_ptr<v3d::event::Bindings> Engine::bindings() const noexcept {
    return bindings_;
}

/**
 **/
bool Engine::initialize() {
    logger_ = boost::make_shared<v3d::log::Logger>();
    features_ = features();

    logger_->get()->info("Initializing engine...");

    const std::string dataPath = appPath_ + std::string("data/");
    assetManager_ = boost::make_shared<v3d::asset::Manager>(dataPath, logger_);
    v3d::asset::media::registerLoaders(*assetManager_, logger_);

    dispatcher_ = boost::make_shared<entt::dispatcher>();
    router_ = boost::make_shared<v3d::event::Router>(dispatcher_);
    commandListener_ = dispatcher_->sink<v3d::event::Event>().connect<&Engine::command>(*this);

    if (features_.has(Feature::Config) && !loadConfig()) {
        return false;
    }
    loadSettings();
    startInput();
    if (features_.has(Feature::Window) && !openWindow()) {
        return false;
    }
    if (!start()) {
        return false;
    }
    // a key bound to a command nothing handles does nothing when pressed. Each such binding
    // is reported here, at startup, rather than only when the key is first pressed
    for (const std::string& command : unhandled()) {
        logger_->get()->error("{} is bound to a key, but nothing handles it", command);
    }
    return true;
}

/**
 **/
bool Engine::loadConfig() {
    manifest_ = boost::make_shared<v3d::asset::Manifest>(logger_);
    if (!manifest_->load(*assetManager_, "config.json")) {
        return false;
    }
    // a binding config is optional: an app with none sends no commands from a key
    const boost::shared_ptr<v3d::asset::kind::Json> mappings = manifest_->get(v3d::event::Bindings::DOCUMENT);
    if (mappings) {
        bindings_ = boost::make_shared<v3d::event::Bindings>(router_, logger_,
            [](const v3d::event::Event& source) {
                const std::string_view device = source.context() ? source.context()->name() : std::string_view();
                if (device == "keyboard") {
                    return v3d::input::isKeyName(source.name());
                }
                if (device == "mouse") {
                    return v3d::input::isButtonName(source.name());
                }
                return true;
            });
        if (!bindings_->load(mappings->document())) {
            return false;
        }
    }
    return true;
}

/**
 **/
void Engine::startInput() {
    v3d::input::DeviceTypes devices;
    if (features_.has(Feature::KeyboardInput)) {
        devices |= v3d::input::DeviceType::Keyboard;
    }
    if (features_.has(Feature::MouseInput)) {
        devices |= v3d::input::DeviceType::Mouse;
    }
    if (!devices.empty()) {
        devices_ = boost::make_shared<v3d::input::Devices>(router_, dispatcher_, devices);
    }
}

/**
 **/
bool Engine::openWindow() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        logger_->get()->error("SDL could not initialize! SDL_Error: {}", SDL_GetError());
        return false;
    }
    // from here on, shutdown() or ~Engine has to destroy the window and quit SDL
    needShutdown_ = true;

    window_ = boost::make_shared<v3d::window::Window>(logger_);

    // a size of -1 leaves the window at its own default, so an app with no window
    // config, or none carrying dimensions, still gets a window
    int width = -1;
    int height = -1;
    if (features_.has(Feature::Config)) {
        const boost::shared_ptr<v3d::asset::kind::Json> windowConfig = manifest_->get(WINDOW_DOCUMENT);
        if (windowConfig) {
            // as with the bindings, a window document that cannot be read makes startup
            // return false rather than throw
            const boost::json::object& doc = windowConfig->document();
            const std::optional<glm::ivec2> size = windowSize(doc);
            if (!size) {
                logger_->get()->error("The window config needs a window with a whole width and height an int can hold");
                return false;
            }
            width = size->x;
            height = size->y;
        }
    }
    return window_->create(glm::ivec2(width, height));
}

/**
 **/
Engine::~Engine() {
    if (needShutdown_) {
        window_->destroy();
        SDL_Quit();
    }
}

/**
 **/
Features Engine::features() const {
    return Feature::Window | Feature::Config | Feature::KeyboardInput | Feature::MouseInput;
}

/**
 **/
const boost::json::object* Engine::document(std::string_view kind) const {
    if (!manifest_) {
        return nullptr;
    }
    const boost::shared_ptr<v3d::asset::kind::Json> held = manifest_->get(kind);
    return held ? &held->document() : nullptr;
}

/**
 **/
Statistics::Scope Engine::measure(std::string_view name) {
    return statistics_.scope(name);
}

/**
 **/
bool Engine::start() {
    return true;
}

/**
 **/
bool Engine::release() {
    return true;
}

/**
 **/
bool Engine::shutdown() {
    // the app's release() runs once, before the window is destroyed: whatever presents to
    // the window has to wait for the device to go idle while the window still exists
    bool released = true;
    if (!released_) {
        released_ = true;
        // when release() throws, ~Engine destroys the window and quits SDL. ~Engine runs
        // after the app's own members are destroyed, so a renderer still goes before the window
        released = release();
    }
    if (!needShutdown_) {
        return released;
    }
    logger_->get()->info("Shutting down engine...");
    window_->destroy();
    SDL_Quit();
    needShutdown_ = false;
    return released;
}

/**
 **/
void Engine::command(const v3d::event::Event& event) {
    if (!commands_.invoke(event)) {
        logger_->get()->warn("nothing handles {}", event.str());
    }
}

/**
 **/
v3d::event::Commands& Engine::commands() noexcept {
    return commands_;
}

/**
 **/
const v3d::event::Commands& Engine::commands() const noexcept {
    return commands_;
}

/**
 **/
std::vector<std::string> Engine::unhandled() const {
    if (!bindings_) {
        return std::vector<std::string>();
    }
    return commands_.unhandled(bindings_->commands());
}

/**
 **/
std::vector<std::string> Engine::unhandled(const boost::json::object& document) const {
    // the bindings install what they read into an event router. They get a private one here,
    // not the one the loop routes through
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    v3d::event::Bindings bindings(boost::make_shared<v3d::event::Router>(dispatcher), boost::make_shared<v3d::log::Logger>());
    if (!bindings.load(document)) {
        return std::vector<std::string>{ "mappings" };
    }
    return commands_.unhandled(bindings.commands());
}

/**
 **/
bool Engine::render() {
    return true;
}

/**
 **/
void Engine::route(const SDL_Event& event) {
    // the app sees the event first, because its ui draws over the scene and the cursor
    // points at the ui first. An event the app consumes never reaches the bindings
    if (!onEvent(event) && devices_ && devices_->filterEvent(event)) {
        return;
    }
    // quit, resize and focus are window events rather than input, so the engine handles
    // them even when the app consumed the event
    handleEvent(event);
}

/**
 **/
bool Engine::onEvent(const SDL_Event& event) {
    (void)event;
    return false;
}

/**
 **/
void Engine::handleEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
    // SDL turns the last window closing into a quit only once that window is destroyed,
    // and nothing here destroys it, so the request is what to act on
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        requestQuit();
        break;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        if (window_) {
            window_->resize(glm::ivec2(event.window.data1, event.window.data2));
        }
        dispatcher_->trigger(v3d::event::kind::WindowResize(glm::ivec2(event.window.data1, event.window.data2)));
        break;
    // a key released while the window is unfocused never arrives. WindowFocus lets an app
    // drop held input when focus goes
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        dispatcher_->trigger(v3d::event::kind::WindowFocus(true));
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        dispatcher_->trigger(v3d::event::kind::WindowFocus(false));
        break;
    case SDL_EVENT_WINDOW_MINIMIZED:
        minimized_ = true;
        break;
    // a window minimized from maximized comes back maximized, which SDL reports instead of a
    // restore
    case SDL_EVENT_WINDOW_RESTORED:
    case SDL_EVENT_WINDOW_MAXIMIZED:
        minimized_ = false;
        break;
    default:
        break;
    }
}

/**
 **/
bool Engine::eventLoop() {
    SDL_Event event;
    // nanoseconds, not SDL_GetTicks(): a whole millisecond cannot express 60 Hz, and a frame
    // faster than 1 ms measures as no elapsed time at all
    uint64_t lastTick = SDL_GetTicksNS();
    // a window can be created minimized, and then sends no event saying so
    if (window_ && window_->created()) {
        minimized_ = (SDL_GetWindowFlags(window_->sdl()) & SDL_WINDOW_MINIMIZED) != 0;
    }
    while (!quitting_) {
        while (SDL_PollEvent(&event) && !quitting_) {
            route(event);
        }
        // an event handler may have asked to stop, and its window may be gone, so nothing
        // after this point runs on the frame that quit
        if (quitting_) {
            break;
        }
        // a minimized window shows nothing, so the loop sleeps until an event arrives instead
        // of presenting frames. The wait is not a frame, and the clock restarts after it
        if (minimized_) {
            if (SDL_WaitEvent(&event)) {
                route(event);
            }
            lastTick = SDL_GetTicksNS();
            continue;
        }
        // tick the game, telling it how long the last frame took
        const uint64_t now = SDL_GetTicksNS();
        const uint64_t elapsed = now - lastTick;
        lastTick = now;
        if (!tick(static_cast<unsigned int>(elapsed / SDL_NS_PER_MS))) {
            return false;
        }
        // advance the simulation by each whole fixed step now due. The accumulator clamps
        // the frame and carries the remainder forward
        statistics_.frame(std::chrono::nanoseconds(elapsed), accumulator_.accumulate(elapsed));
        while (accumulator_.drain()) {
            if (!step(Accumulator::seconds)) {
                return false;
            }
        }
        if (!render()) {
            return false;
        }
        // the edges belong to this frame, and everything that reads them has now run. The
        // loop clears them so that an app does not have to
        if (devices_) {
            devices_->flush();
        }
    }
    return true;
}

/**
 **/
const v3d::input::KeyState* Engine::keys() const {
    return devices_ ? devices_->keys() : nullptr;
}

/**
 **/
const v3d::input::MouseState* Engine::mouse() const {
    return devices_ ? devices_->mouse() : nullptr;
}

/**
 **/
bool Engine::held(std::string_view command) const {
    const v3d::input::KeyState* state = keys();
    if (!bindings_ || state == nullptr) {
        return false;
    }
    return std::ranges::any_of(bindings_->sources(command),
        [state](const v3d::event::Event& source) { return state->held(source.name()); });
}

/**
 **/
bool Engine::tick(unsigned int /* delta */) {
    return true;
}

/**
 **/
bool Engine::step(float seconds) {
    for (const Snapshot snapshot : snapshots_) {
        snapshot(registry_);
    }
    return simulate(seconds);
}

/**
 **/
bool Engine::simulate(float /* step */) {
    return true;
}

float Engine::alpha() const noexcept {
    return accumulator_.alpha();
}

const Statistics& Engine::statistics() const noexcept {
    return statistics_;
}

boost::shared_ptr<v3d::window::Window> Engine::window() const {
    return window_;
}

const boost::shared_ptr<v3d::log::Logger>& Engine::logger() const noexcept {
    return logger_;
}

const boost::shared_ptr<v3d::asset::Manifest>& Engine::manifest() const noexcept {
    return manifest_;
}

const boost::shared_ptr<v3d::asset::Manager>& Engine::assets() const noexcept {
    return assetManager_;
}

const boost::shared_ptr<entt::dispatcher>& Engine::dispatcher() const noexcept {
    return dispatcher_;
}

entt::registry& Engine::registry() noexcept {
    return registry_;
}

const entt::registry& Engine::registry() const noexcept {
    return registry_;
}

const boost::shared_ptr<v3d::event::Router>& Engine::events() const noexcept {
    return router_;
}

};  // namespace v3d::engine
