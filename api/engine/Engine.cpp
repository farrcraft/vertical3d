/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

#include <SDL3/SDL.h>

#include <string>

#include "Feature.h"
#include "../input/DeviceType.h"
#include "../event/WindowResize.h"

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

namespace v3d::engine {
/**
 **/
Engine::Engine(const std::string& appPath) :
    appPath_(appPath),
    features_(0),
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

bool Engine::registerEventMappings() {
    boost::shared_ptr<v3d::asset::Json> mappingConfig = config_->get(v3d::config::Type::Binding);
    if (!mappingConfig) {
        return true;
    }

    // We're only supporting a single global mapper for now
    boost::shared_ptr<v3d::event::Mapper> mapper = boost::make_shared<v3d::event::Mapper>("global");

    auto const doc = mappingConfig->document();
    // every lookup below is guarded by a contains() rather than reaching straight for
    // at(): boost::json::at throws, and a mapping document this function does not
    // understand has to come back as a false return, not as an exception out of startup.
    if (!doc.contains("mappings") || !doc.at("mappings").is_array()) {
        logger_->get()->error("Missing mappings in config");
        return false;
    }
    auto const items = doc.at("mappings").as_array();
    auto it = items.begin();
    for (; it != items.end(); ++it) {
        if (!it->is_object()) {
            logger_->get()->error("Unrecognized mapping");
            return false;
        }
        auto const mapping = it->as_object();
        if (!mapping.contains("source") || !mapping.at("source").is_object()) {
            logger_->get()->error("Missing mapping source");
            return false;
        }
        auto const source = mapping.at("source");
        if (!source.as_object().contains("name") || !source.as_object().contains("context")) {
            logger_->get()->error("Mapping source needs both a name and a context");
            return false;
        }
        std::string sourceName = boost::json::value_to<std::string>(source.at("name"));
        std::string sourceContextName = boost::json::value_to<std::string>(source.at("context"));
        boost::shared_ptr<v3d::event::Context> sourceContext = eventEngine_->resolveContext(sourceContextName);
        v3d::event::Event sourceEvent(sourceName, sourceContext);
        sourceEvent.type(v3d::event::Type::Source);
        // an optional "state" binds one edge only - "pressed"/"down" or "released"/"up".
        // without it the binding matches both, which is what most actions want.
        if (source.as_object().contains("state")) {
            std::string sourceState = boost::json::value_to<std::string>(source.at("state"));
            sourceEvent.state(v3d::event::stringToState(sourceState));
        }

        if (!mapping.contains("destination") || !mapping.at("destination").is_object()) {
            logger_->get()->error("Missing mapping destination");
            return false;
        }
        auto const destination = mapping.at("destination");
        if (!destination.as_object().contains("name") || !destination.as_object().contains("context")) {
            logger_->get()->error("Mapping destination needs both a name and a context");
            return false;
        }
        std::string destinationName = boost::json::value_to<std::string>(destination.at("name"));
        std::string destinationContextName = boost::json::value_to<std::string>(destination.at("context"));
        boost::shared_ptr<v3d::event::Context> destinationContext = eventEngine_->resolveContext(destinationContextName);
        v3d::event::Event destinationEvent(destinationName, destinationContext);
        destinationEvent.type(v3d::event::Type::Destination);
        // an optional "param" lets one action serve several bindings, telling them apart by
        // the value it arrives with. It reaches the handler as the event's data, the same
        // way a menu item's value does.
        if (destination.as_object().contains("param")) {
            auto const param = destination.at("param");
            if (param.is_int64()) {
                destinationEvent.data(static_cast<int>(param.as_int64()));
            } else if (param.is_bool()) {
                destinationEvent.data(param.as_bool());
            } else if (param.is_string()) {
                destinationEvent.data(boost::json::value_to<std::string>(param));
            } else {
                logger_->get()->error("Unsupported binding param type for [{}]", destinationName);
                return false;
            }
        }
        mapper->map(sourceEvent, destinationEvent);
    }
    eventEngine_->addMapper(mapper);
    return true;
}

/**
 **/
bool Engine::initialize(int features) {
    logger_ = boost::make_shared<v3d::log::Logger>();
    features_ = features;

    logger_->get()->info("Initializing engine...");

    std::string dataPath = appPath_ + std::string("data/");
    assetManager_ = boost::make_shared<v3d::asset::Manager>(dataPath, logger_);

    dispatcher_ = boost::make_shared<entt::dispatcher>();
    eventEngine_ = boost::make_shared<v3d::event::Engine>(dispatcher_);

    if (features_ & Feature::Config) {
        config_ = boost::make_shared<v3d::config::Config>(logger_);
        // Load config (through the asset manager)
        if (!config_->load(assetManager_)) {
            return false;
        }
        // If config includes event mappings/bindings, they will get loaded here
        if (!registerEventMappings()) {
            return false;
        }
    }

    int devices = 0;
    if (features_ & Feature::KeyboardInput) {
        devices |= v3d::input::DeviceType::Keyboard;
    }
    if (features_ & Feature::MouseInput) {
        devices |= v3d::input::DeviceType::Mouse;
    }
    if (devices != 0) {
        inputEngine_ = boost::make_shared<v3d::input::Engine>(eventEngine_, dispatcher_, devices);
    }

    if (features_ & Feature::Window) {
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
        if (features_ & Feature::Config) {
            boost::shared_ptr<v3d::asset::Json> windowConfig = config_->get(v3d::config::Type::Window);
            if (windowConfig) {
                auto const doc = windowConfig->document();
                auto const window = doc.at("window");
                width = boost::json::value_to<int>(window.at("width"));
                height = boost::json::value_to<int>(window.at("height"));
            }
        }
        if (!window_->create(width, height)) {
            return false;
        }
    }
    return true;
}

/**
 **/
bool Engine::shutdown() {
    if (!needShutdown_) {
        return true;
    }
    logger_->get()->info("Shutting down engine...");
    if (features_ & Feature::Window) {
        window_->destroy();
        SDL_Quit();
    }
    return true;
}

/**
 **/
bool Engine::render() {
    return true;
}

/**
 **/
bool Engine::eventLoop() {
    SDL_Event event;
    uint64_t lastTick = SDL_GetTicks();
    // Enter main game loop
    while (!quitting_) {
        // Handle events on queue
        while (SDL_PollEvent(&event) != 0 && !quitting_) {
            // check for input device events first
            if (inputEngine_ && inputEngine_->filterEvent(event)) {
                continue;
            }
            switch (event.type) {
            case SDL_EVENT_QUIT:
                quit();
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                // SDL turns the last window closing into a quit only once that window is
                // destroyed, and nothing here destroys it, so the request is what to act on
                quit();
                break;
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                if (window_) {
                    window_->resize(event.window.data1, event.window.data2);
                }
                dispatcher_->trigger(v3d::event::WindowResize(event.window.data1, event.window.data2));
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                break;
            }
        }
        // an event handler may have asked to stop, and the window it drew into can have
        // gone with it - so nothing after this point runs on the frame that quit
        if (quitting_) {
            break;
        }
        // tick the game, telling it how long the last frame took
        uint64_t now = SDL_GetTicks();
        unsigned int delta = static_cast<unsigned int>(now - lastTick);
        lastTick = now;
        if (!tick(delta)) {
            return false;
        }
        // and draw the frame on the screen
        if (!render()) {
            return false;
        }
    }
    return true;
}

/**
 **/
bool Engine::tick(unsigned int /* delta */) {
    return true;
}

boost::shared_ptr<v3d::render::realtime::Window> Engine::window() const {
    return window_;
}

};  // namespace v3d::engine
