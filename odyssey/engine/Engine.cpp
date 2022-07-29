/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Engine.h"

#include <string>

#include "../../api/engine/Feature.h"
#include "../render/renderable/Player.h"

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

#include <SDL.h>

namespace odyssey::engine {
    /**
     **/
    Engine::Engine(const std::string_view& appPath) :
        v3d::engine::Engine(appPath) {
    }

    /**
     **/
    bool Engine::initialize() {
        if (!v3d::engine::Engine::initialize(static_cast<int>(
            v3d::engine::Engine::Feature::Config |
            v3d::engine::Engine::Feature::Window |
            v3d::engine::Engine::Feature::MouseInput |
            v3d::engine::Engine::Feature::KeyboardInput))) {
            return false;
        }

        player_ = boost::make_shared<Player>(registry_);

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
        dispatcher_->sink<v3d::event::WindowResize>().connect<&odyssey::render::Engine::resize>(renderEngine_);

        return true;
    }

    /**
     **/
    bool Engine::shutdown() {
        if (!v3d::engine::Engine::shutdown()) {
            return false;
        }
        return true;
    }

    /**
     **/
    bool Engine::render() {
        if (!v3d::engine::Engine::render()) {
            return false;
        }
        renderEngine_->renderFrame();
    }

    /**
     **/
    bool Engine::tick() {
        if (!v3d::engine::Engine::tick()) {
            return false;

        }
        // Tick various systems, e.g. Movement System, Collision System, Combat System, etc
        if (!movementSystem_->tick()) {
            return false;
        }
        return true;
    }

};  // namespace odyssey::engine
