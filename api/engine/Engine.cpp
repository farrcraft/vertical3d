/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

#include <api/asset/media/Loaders.h>
#include <api/event/kind/WindowFocus.h>
#include <api/event/kind/WindowResize.h>
#include <api/input/DeviceType.h>
#include <api/input/Keyboard.h>
#include <api/input/Mouse.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <map>
#include <string>

#include "Feature.h"

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

namespace v3d::engine {
/**
 **/
Engine::Engine(const std::string& appPath) :
    appPath_(appPath),
    needShutdown_(false),
    quitting_(false) {
}

/**
 **/
void Engine::quit() noexcept {
    quitting_ = true;
}

/**
 **/
bool Engine::quitting() const noexcept {
    return quitting_;
}

/**
 **/
bool Engine::rebind(const std::string& command, const std::string& key) {
    return bindings_ && bindings_->rebind(command, key);
}

/**
 **/
bool Engine::initialize() {
    logger_ = boost::make_shared<v3d::log::Logger>();
    features_ = features();

    logger_->get()->info("Initializing engine...");

    std::string dataPath = appPath_ + std::string("data/");
    assetManager_ = boost::make_shared<v3d::asset::Manager>(dataPath, logger_);
    v3d::asset::media::registerLoaders(*assetManager_, logger_);

    dispatcher_ = boost::make_shared<entt::dispatcher>();
    eventEngine_ = boost::make_shared<v3d::event::Engine>(dispatcher_);
    quitCommand_ = dispatcher_->sink<v3d::event::Event>().connect<&Engine::command>(*this);

    if (features_.has(Feature::Config) && !loadConfig()) {
        return false;
    }
    startInput();
    if (features_.has(Feature::Window) && !openWindow()) {
        return false;
    }
    return start();
}

/**
 **/
bool Engine::loadConfig() {
    config_ = boost::make_shared<v3d::config::Config>(logger_);
    // Load config (through the asset manager)
    if (!config_->load(assetManager_)) {
        return false;
    }
    // a binding config is optional: an app with none sends no commands from a key
    const boost::shared_ptr<v3d::asset::kind::Json> mappings = config_->get(v3d::config::Type::Binding);
    if (mappings) {
        bindings_ = boost::make_shared<v3d::event::Bindings>(eventEngine_, logger_,
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
        inputEngine_ = boost::make_shared<v3d::input::Engine>(eventEngine_, dispatcher_, devices);
    }
}

/**
 **/
bool Engine::openWindow() {
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        logger_->get()->error("SDL could not initialize! SDL_Error: {}", SDL_GetError());
        return false;
    }
    // We've reached a point of initialization that will require a shutdown
    needShutdown_ = true;

    window_ = boost::make_shared<v3d::render::realtime::Window>(logger_);

    // a size of -1 leaves the window at its own default, so an app with no window
    // config, or none carrying dimensions, still gets a window
    int width = -1;
    int height = -1;
    if (features_.has(Feature::Config)) {
        boost::shared_ptr<v3d::asset::kind::Json> windowConfig = config_->get(v3d::config::Type::Window);
        if (windowConfig) {
            // guarded as the bindings are: a window document this does not understand
            // is a false return out of startup, not an exception out of it
            const boost::json::object& doc = windowConfig->document();
            const boost::json::object* window = doc.contains("window") ? doc.at("window").if_object() : nullptr;
            if (window == nullptr || !window->contains("width") || !window->contains("height") ||
                !window->at("width").is_int64() || !window->at("height").is_int64()) {
                logger_->get()->error("The window config needs a window with a whole width and height");
                return false;
            }
            width = static_cast<int>(window->at("width").as_int64());
            height = static_cast<int>(window->at("height").as_int64());
        }
    }
    return window_->create(width, height);
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
const boost::json::object* Engine::document(v3d::config::Type type) const {
    return document(v3d::config::typeName(type));
}

/**
 **/
const boost::json::object* Engine::document(std::string_view type) const {
    if (!config_) {
        return nullptr;
    }
    const boost::shared_ptr<v3d::asset::kind::Json> held = config_->get(type);
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
    // the app's, once, and before the window: what presents to the window has to let the
    // device go idle while it still exists
    bool released = true;
    if (!released_) {
        released_ = true;
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
    if (event.str() == "ui::quit") {
        quit();
    }
}

/**
 **/
bool Engine::render() {
    return true;
}

/**
 **/
void Engine::route(const SDL_Event& event) {
    // the app is the outer layer - it drew over the scene, so it is what the cursor is
    // pointing at - and what it takes never reaches the bindings, per ADR-0043
    if (!onEvent(event) && inputEngine_ && inputEngine_->filterEvent(event)) {
        return;
    }
    // quit, resize and focus are window facts rather than input, so they are not an app's
    // to decline and not a binding's to consume
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
/**
 **/
void Engine::handleEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
    // SDL turns the last window closing into a quit only once that window is destroyed,
    // and nothing here destroys it, so the request is what to act on
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        quit();
        break;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        if (window_) {
            window_->resize(event.window.data1, event.window.data2);
        }
        dispatcher_->trigger(v3d::event::kind::WindowResize(event.window.data1, event.window.data2));
        break;
    // a key released while the window is unfocused never arrives, so an app that wants
    // held input dropped needs to be told focus went rather than poll for it
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        dispatcher_->trigger(v3d::event::kind::WindowFocus(true));
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        dispatcher_->trigger(v3d::event::kind::WindowFocus(false));
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
    // Enter main game loop
    while (!quitting_) {
        // Handle events on queue
        while (SDL_PollEvent(&event) != 0 && !quitting_) {
            route(event);
        }
        // an event handler may have asked to stop, and the window it drew into can have
        // gone with it - so nothing after this point runs on the frame that quit
        if (quitting_) {
            break;
        }
        // tick the game, telling it how long the last frame took
        uint64_t now = SDL_GetTicksNS();
        uint64_t elapsed = now - lastTick;
        lastTick = now;
        if (!tick(static_cast<unsigned int>(elapsed / SDL_NS_PER_MS))) {
            return false;
        }
        // and advance the simulation by however many whole steps that frame owes, per
        // ADR-0032 - the accumulator clamps the frame and carries the remainder forward
        statistics_.frame(elapsed, accumulator_.accumulate(elapsed));
        while (accumulator_.drain()) {
            if (!simulate(Accumulator::seconds)) {
                return false;
            }
        }
        // and draw the frame on the screen
        if (!render()) {
            return false;
        }
        // the edges belonged to this frame, and everything that reads them has now run. The
        // loop is what clears them, so "exactly once per frame" is not a precondition an app
        // has to honour
        if (inputEngine_) {
            inputEngine_->flush();
        }
    }
    return true;
}

/**
 **/
const v3d::input::KeyState* Engine::keys() const {
    return inputEngine_ ? inputEngine_->keys() : nullptr;
}

/**
 **/
const v3d::input::MouseState* Engine::mouse() const {
    return inputEngine_ ? inputEngine_->mouse() : nullptr;
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
bool Engine::simulate(float /* step */) {
    return true;
}

float Engine::alpha() const noexcept {
    return accumulator_.alpha();
}

const Statistics& Engine::statistics() const noexcept {
    return statistics_;
}

boost::shared_ptr<v3d::render::realtime::Window> Engine::window() const {
    return window_;
}

const boost::shared_ptr<v3d::log::Logger>& Engine::logger() const noexcept {
    return logger_;
}

const boost::shared_ptr<v3d::config::Config>& Engine::config() const noexcept {
    return config_;
}

const boost::shared_ptr<v3d::asset::Manager>& Engine::assets() const noexcept {
    return assetManager_;
}

const boost::shared_ptr<entt::dispatcher>& Engine::dispatcher() const noexcept {
    return dispatcher_;
}

const boost::shared_ptr<v3d::event::Engine>& Engine::events() const noexcept {
    return eventEngine_;
}

};  // namespace v3d::engine
