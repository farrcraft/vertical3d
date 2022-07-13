/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Engine.h"

#include "../render/renderable/Player.h"

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

#include <SDL.h>

namespace odyssey::engine {

    /**
     **/
    bool Engine::initialize() {
        logger_ = boost::make_shared<Logger>();

        LOG_INFO(logger_) << "Initializing engine...";

        config_ = boost::make_shared<odyssey::config::Config>(logger_);
        dispatcher_ = boost::make_shared<entt::dispatcher>();

        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            LOG_ERROR(logger_) << "SDL could not initialize! SDL_Error: " << SDL_GetError();
            return false;
        }

        // Load bootstrap config
        if (!bootstrap_.load(logger_)) {
            return false;
        }

        assetManager_ = boost::make_shared<odyssey::asset::Manager>(bootstrap_.dataPath(), logger_);

        player_ = boost::make_shared<Player>(registry_);

        window_ = boost::make_shared<odyssey::ui::Window>(logger_);
        if (!window_->create(bootstrap_.windowWidth(), bootstrap_.windowHeight())) {
            return false;
        }

        if (!config_->load(assetManager_)) {
            return false;
        }

        inputEngine_ = boost::make_shared<odyssey::input::Engine>(dispatcher_);

        movementSystem_ = boost::make_shared<odyssey::system::Movement>();

        renderEngine_ = boost::make_shared<odyssey::render::Engine>(logger_, assetManager_);
        if (!renderEngine_->initialize(window_)) {
            return false;
        }

        // need to convert this to ECS...
        renderEngine_->scene()->setPlayer(boost::make_shared<odyssey::render::renderable::Player>(renderEngine_, player_));

        /*
        we don't want to send device events directly to systems
        the input needs to be resolved to an action via the bindings config
        there could also be multiple binding contexts that map the same event to different actions
        not all contexts will be actively able to process all events all of the time
        sometimes a context could block others (e.g. an active ui blocks player interaction)
        and sometimes multiple contexts will need to all process the same event

            // Assign events to systems.
            dispatcher_->sink<odyssey::event::KeyDown>().connect<&odyssey::system::Movement::on_key_down>(movementSystem_);
            dispatcher_->sink<odyssey::event::KeyUp>().connect<&odyssey::system::Movement::on_key_up>(movementSystem_);

            // Assign events to window.
            dispatcher_->sink<odyssey::event::KeyDown>().connect<&Window::on_key_down>(window_);
        */
        return true;
    }

    /**
     **/
    bool Engine::shutdown() {
        LOG_INFO(logger_) << "Shutting down engine...";

        window_->destroy();

        // Quit SDL subsystems
        SDL_Quit();
        return true;
    }

    /**
     **/
    bool Engine::run() {
        bool quit = false;
        SDL_Event event;
        // Enter main game loop
        while (!quit) {
            // Handle events on queue
            while (SDL_PollEvent(&event) != 0) {
                // check for input device events first
                if (inputEngine_->filterEvent(event)) {
                    continue;
                }
                switch (event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                        renderEngine_->resize(event.window.data1, event.window.data2);
                        break;
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        renderEngine_->resize(event.window.data1, event.window.data2);
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
            renderEngine_->renderFrame();
        }
        return true;
    }

    /**
     **/
    bool Engine::tick() {
        // Tick various systems, e.g. Movement System, Collision System, Combat System, etc
        if (!movementSystem_->tick()) {
            return false;
        }
        return true;
    }

};  // namespace odyssey::engine
