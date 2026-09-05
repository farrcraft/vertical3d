/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Engine.h"
#include "Unit.h"

#include <SDL3/SDL.h>

#include <string>

#include "../../api/engine/Feature.h"

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
        v3d::engine::Feature::Window |
        v3d::engine::Feature::MouseInput |
        v3d::engine::Feature::KeyboardInput))) {
        return false;
    }

    window_->caption("Odyssey");

    player_ = boost::make_shared<Player>(&registry_);

    movementSystem_ = boost::make_shared<odyssey::system::Movement>(&registry_);

    renderer_ = boost::make_shared<odyssey::render::Renderer>(window(), logger_, assetManager_, &registry_);
    renderer_->player(player_);

    // one sink for every mapped event: a device event is resolved to an action by the
    // bindings before it gets here, so nothing subscribes to a key
    dispatcher_->sink<v3d::event::Event>().connect<&Engine::handleEvent>(*this);

    return true;
}

/**
 **/
void Engine::handleEvent(const v3d::event::Event& event) {
    if (event.context()->name() != "odyssey") {
        return;
    }
    if (event.name() == "quit") {
        // not shutdown() - the event loop ticks and renders once more after a handler
        // returns, and that frame would be drawn into a destroyed window
        quit();
    }
}

/**
 **/
bool Engine::shutdown() {
    if (renderer_) {
        // the device has to be idle before the window it presents to is destroyed
        renderer_->shutdown();
    }
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
    renderer_->draw();
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
