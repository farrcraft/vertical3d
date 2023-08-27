/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Engine.h"
#include "Context.h"
#include "Window3D.h"

namespace v3d::render::realtime {
    /* A 3D render engine.
     **/
    class Engine3D : public Engine {
    public:
        /**
         **/
        Engine3D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry& registry);

        /**
         **/
        bool initialize(const boost::shared_ptr<Window3D>& window);

        /**
         **/
        void renderFrame();

        boost::shared_ptr<Context> context();

    private:
        boost::shared_ptr<Context> context_;
    };
};  // namespace v3d::render::realtime
