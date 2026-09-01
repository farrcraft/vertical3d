/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Engine.h"
#include "Unit.h"

#include <SDL3/SDL.h>

#include <string>

#include "../../api/engine/Feature.h"
#include "../render/renderable/Player.h"

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

namespace odyssey::engine {
    /**
     **/
    Engine::Engine(const std::string& appPath) :
        v3d::engine::Engine(appPath) {
    }

    /**
     **/
    bool Engine::initialize() {
        if (!v3d::engine::Engine::initialize(static_cast<int>(
            v3d::engine::Feature::Config |
            v3d::engine::Feature::Window2D |
            v3d::engine::Feature::MouseInput |
            v3d::engine::Feature::KeyboardInput))) {
            return false;
        }

        player_ = boost::make_shared<Player>(&registry_);

        movementSystem_ = boost::make_shared<odyssey::system::Movement>(&registry_);

        renderEngine_ = boost::make_shared<v3d::render::realtime::Engine2D>(logger_, assetManager_, &registry_);

        boost::shared_ptr<v3d::render::realtime::Window2D> window2D =
            boost::dynamic_pointer_cast<v3d::render::realtime::Window2D>(window());
        if (!window2D) {
            return false;
        }
        int width = unit::tile_width * unit::screen_tile_width;
        int height = unit::tile_height * unit::screen_tile_height;
        window2D->logicalSize(width, height);

        if (!renderEngine_->initialize(window2D)) {
            return false;
        }

        // the engine installs an empty scene; replace it with ours so the player is drawn.
        // need to convert this to ECS...
        boost::shared_ptr<odyssey::render::Scene> scene = boost::make_shared<odyssey::render::Scene>(renderEngine_->context());
        scene->setPlayer(boost::make_shared<odyssey::render::renderable::Player>(renderEngine_, player_));
        renderEngine_->scene(scene);

        /*
        we don't want to send device events directly to systems
        the input needs to be resolved to an action via the bindings config
        there could also be multiple binding contexts that map the same event to different actions
        not all contexts will be actively able to process all events all of the time
        sometimes a context could block others (e.g. an active ui blocks player interaction)
        and sometimes multiple contexts will need to all process the same event

        the input engine maintains current key state

        BindingContext
        Binding
        BindingResolver


            // Assign events to systems.
            dispatcher_->sink<odyssey::event::KeyDown>().connect<&odyssey::system::Movement::on_key_down>(movementSystem_);
            dispatcher_->sink<odyssey::event::KeyUp>().connect<&odyssey::system::Movement::on_key_up>(movementSystem_);

            // Assign events to window.
            dispatcher_->sink<odyssey::event::KeyDown>().connect<&Window::on_key_down>(window_);
        */
        dispatcher_->sink<v3d::event::WindowResize>().connect<&v3d::render::realtime::Engine::resize>(*renderEngine_);

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
        return true;
    }

    /**
     **/
    bool Engine::tick(unsigned int delta) {
        if (!v3d::engine::Engine::tick(delta)) {
            return false;
        }
        // Tick various systems, e.g. Movement System, Collision System, Combat System, etc
        if (!movementSystem_->tick()) {
            return false;
        }
        return true;
    }

};  // namespace odyssey::engine
