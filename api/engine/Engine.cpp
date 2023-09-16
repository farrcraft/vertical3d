/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

#include <string>

#include "Feature.h"
#include "../input/DeviceType.h"
#include "../event/WindowResize.h"
#include "../render/realtime/2D/Window2D.h"
#include "../render/realtime/Window3D.h"

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

#include <SDL.h>

namespace v3d::engine {
    /**
     **/
    Engine::Engine(const std::string& appPath) :
        appPath_(appPath),
        features_(0),
        needShutdown_(false) {
    }

    bool Engine::registerEventMappings() {
        boost::shared_ptr<v3d::asset::Json> mappingConfig = config_->get(v3d::config::Type::Binding);
        if (!mappingConfig) {
            return true;
        }

        // We're only supporting a single global mapper for now
        boost::shared_ptr<v3d::event::Mapper> mapper = boost::make_shared<v3d::event::Mapper>("global");

        auto const doc = mappingConfig->document();
        auto const mappings = doc.at("mappings");
        if (!mappings.is_array()) {
            LOG_ERROR(logger_) << "Missing mappings in config";
            return false;
        }
        // for each mapping
        auto const items = mappings.as_array();
        auto it = items.begin();
        for (; it != items.end(); ++it) {
            if (!it->is_object()) {
                LOG_ERROR(logger_) << "Unrecognized mapping";
                return false;
            }
            auto const mapping = it->as_object();
            auto const source = mapping.at("source");
            if (!source.is_object()) {
                LOG_ERROR(logger_) << "Missing mapping source";
                return false;
            }
            std::string sourceName = boost::json::value_to<std::string>(source.at("name"));
            std::string sourceContextName = boost::json::value_to<std::string>(source.at("context"));
            boost::shared_ptr<v3d::event::Context> sourceContext = eventEngine_->resolveContext(sourceContextName);
            v3d::event::Event sourceEvent(sourceName, sourceContext);
            sourceEvent.type(v3d::event::Type::Source);

            auto const destination = mapping.at("destination");
            if (!destination.is_object()) {
                LOG_ERROR(logger_) << "Missing mapping destination";
                return false;
            }
            std::string destinationName = boost::json::value_to<std::string>(destination.at("name"));
            std::string destinationContextName = boost::json::value_to<std::string>(destination.at("context"));
            boost::shared_ptr<v3d::event::Context> destinationContext = eventEngine_->resolveContext(destinationContextName);
            v3d::event::Event destinationEvent(destinationName, destinationContext);
            destinationEvent.type(v3d::event::Type::Destination);
            mapper->map(sourceEvent, destinationEvent);
        }
        eventEngine_->addMapper(mapper);
        return true;
    }

    /**
     **/
    bool Engine::initialize(int features) {
        logger_ = boost::make_shared<v3d::log::Logger>();

        LOG_INFO(logger_) << "Initializing engine...";

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

        if (features_ & Feature::Window2D || features_ & Feature::Window3D) {
            // Initialize SDL
            if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                LOG_ERROR(logger_) << "SDL could not initialize! SDL_Error: " << SDL_GetError();
                return false;
            }
            // We've reached a point of initialization that will require a shutdown
            needShutdown_ = true;

            if (features & Feature::Window2D) {
                window_ = boost::make_shared<v3d::render::realtime::Window2D>(logger_);
            } else {
                window_ = boost::make_shared<v3d::render::realtime::Window3D>(logger_);
            }

            // if there is a window config with dimensions specified, we'll use those.
            if (features_ & Feature::Config) {
                boost::shared_ptr<v3d::asset::Json> windowConfig = config_->get(v3d::config::Type::Window);
                int width = -1;
                int height = -1;
                int logicalWidth = -1;
                int logicalHeight = -1;
                if (windowConfig) {
                    auto const doc = windowConfig->document();
                    auto const window = doc.at("window");
                    width = boost::json::value_to<int>(window.at("width"));
                    height = boost::json::value_to<int>(window.at("height"));
                    logicalWidth = boost::json::value_to<int>(window.at("logicalWidth"));
                    logicalHeight = boost::json::value_to<int>(window.at("logicalHeight"));
                }
                if (features & Feature::Window2D) {
                    if (!boost::static_pointer_cast<v3d::render::realtime::Window2D>(window_)->create(width, height, logicalWidth, logicalHeight)) {
                        return false;
                    }
                } else {
                    if (!window_->create(width, height)) {
                        return false;
                    }
                }
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
        LOG_INFO(logger_) << "Shutting down engine...";
        if (features_ & Feature::Window2D || features_ & Feature::Window3D) {
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
        bool quit = false;
        SDL_Event event;
        // Enter main game loop
        while (!quit) {
            // Handle events on queue
            while (SDL_PollEvent(&event) != 0) {
                // check for input device events first
                if (inputEngine_ && inputEngine_->filterEvent(event)) {
                    continue;
                }
                switch (event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        if (window_) {
                            window_->resize(event.window.data1, event.window.data2);
                        }
                        dispatcher_->trigger(v3d::event::WindowResize(event.window.data1, event.window.data2));
                        break;
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        break;
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        break;
                    }
                    break;
                }
            }
            // tick the game
            if (!tick()) {
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
    bool Engine::tick() {
        return true;
    }

};  // namespace v3d::engine
