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
    Engine3D::Engine3D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry& registry) :
        Engine(logger, assetManager, registry) {
        context_ = boost::make_shared<Context>();
    }

    /**
     **/
    bool Engine3D::initialize(const boost::shared_ptr <Window3D>& window) {
        Engine::initialize(window);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glDepthRange(0.0f, 1.0f);
        glEnable(GL_DEPTH_CLAMP);
        // CCW winding is default
        glFrontFace(GL_CCW);
        glActiveTexture(GL_TEXTURE0);

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
