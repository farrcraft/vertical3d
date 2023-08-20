/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Engine.h"

#include <boost/make_shared.hpp>


namespace v3d::render::realtime {
    /**
     **/
    Engine::Engine(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry& registry) :
        logger_(logger),
        assetManager_(assetManager),
        registry_(registry) {
    }

    /**
     **/
    Engine::~Engine() {
    }

    bool Engine::initialize(const boost::shared_ptr<Window>& window) {
        window_ = window;
        return true;
    }

    /**
     **/
    bool Engine::shutdown() {
        return true;
    }

    /**
     **/
    void Engine::resize(const v3d::event::WindowResize& event) {
        window_->resize(event.width(), event.height());
        //  backBuffer_ = boost::make_shared<Texture>(context_, width, height);
    }

    /**
     **/
    boost::shared_ptr<v3d::asset::Manager> Engine::assetManager() {
        return assetManager_;
    }

    boost::shared_ptr<Window> Engine::window() {
        return window_;
    }
};  // namespace v3d::render::realtime
