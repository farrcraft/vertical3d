/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Engine3D.h"

#include <GL/glew.h>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {
    /**
     **/
    Engine3D::Engine3D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry) :
        Engine(logger, assetManager, registry) {
    }

    /**
     **/
    bool Engine3D::initialize(const boost::shared_ptr <Window3D>& window) {
        Engine::initialize(window);

        // the context can only be built once there is a created window to take a device from
        context_ = boost::make_shared<Context3D>(logger(), window);

        return true;
    }

    boost::shared_ptr<Context> Engine3D::context() {
        return context_;
    }

    /**
     **/
    void Engine3D::renderFrame() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

};  // namespace v3d::render::realtime
