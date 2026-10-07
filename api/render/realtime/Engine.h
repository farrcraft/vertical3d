/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/log/Logger.h>

#include "Window.h"

namespace v3d::render::realtime {
/**
 * The render engine. This is separate from the game engine: the game engine coordinates the
 * game, and the render engine manages the rendering pipeline.
 **/
class Engine {
 public:
    /**
     **/
    Engine(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager);

    /**
     **/
    virtual ~Engine();

    virtual bool initialize(const boost::shared_ptr<Window>& window);

    /**
     **/
    virtual bool shutdown();

    /**
     **/
    boost::shared_ptr<v3d::asset::Manager> assetManager();

    boost::shared_ptr<Window> window();

    /**
     **/
    virtual void renderFrame() = 0;

 protected:
    /**
     **/
    const boost::shared_ptr<v3d::log::Logger>& logger() const noexcept;

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<v3d::asset::Manager> assetManager_;
    boost::shared_ptr<Window> window_;
};
};  // namespace v3d::render::realtime
