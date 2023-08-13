/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

#include <string>

#include "Feature.h"
#include "../input/DeviceType.h"
#include "../event/WindowResize.h"

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

#include <SDL.h>

namespace v3d::engine {
    /**
     **/
    Engine::Engine(const std::string_view& appPath) :
        appPath_(appPath),
        features_(0) {
    }

    /**
     **/
    bool Engine::initialize(int features) {
        logger_ = boost::make_shared<v3d::log::Logger>();

        LOG_INFO(logger_) << "Initializing engine...";

        std::string dataPath = appPath_ + std::string("data/");
        assetManager_ = boost::make_shared<v3d::asset::Manager>(dataPath, logger_);

        if (features_ & Feature::Config) {
            config_ = boost::make_shared<v3d::config::Config>(logger_);
            // Load config (through the asset manager)
            if (!config_->load(assetManager_)) {
                return false;
            }
        }

        dispatcher_ = boost::make_shared<entt::dispatcher>();
        int devices = 0;
        if (features_ & Feature::KeyboardInput) {
            devices |= v3d::input::DeviceType::Keyboard;
        }
        if (features_ & Feature::MouseInput) {
            devices |= v3d::input::DeviceType::Mouse;
        }
        if (devices != 0) {
            inputEngine_ = boost::make_shared<v3d::input::Engine>(dispatcher_, devices);
        }

        if (features_ & Feature::Window) {
            // Initialize SDL
            if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                LOG_ERROR(logger_) << "SDL could not initialize! SDL_Error: " << SDL_GetError();
                return false;
            }

            int width = -1;
            int height = -1;
            // if there is a window config with dimensions specified, we'll use those.
            if (features_ & Feature::Config) {
                boost::shared_ptr<v3d::asset::Json> windowConfig = config_->get(v3d::config::Type::Window);
                if (windowConfig) {
                    auto const doc = windowConfig->document();
                    auto const window = doc.at("window");
                    width = boost::json::value_to<int>(window.at("width"));
                    height = boost::json::value_to<int>(window.at("height"));
                }
            }
            window_ = boost::make_shared<v3d::ui::Window>(logger_);
            if (features_ & Feature::Config && !window_->create(width, height)) {
                return false;
            }
        }

        return true;
    }

    /**
     **/
    bool Engine::shutdown() {
        LOG_INFO(logger_) << "Shutting down engine...";
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
