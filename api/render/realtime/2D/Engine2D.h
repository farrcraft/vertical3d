/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

//#include <list>

#include "Context2D.h"
#include "../Operation.h"
//#include "Renderable.h"
#include "Scene2D.h"
#include "../Engine.h"
#include "Texture2DCache.h"
#include "Window2D.h"

namespace v3d::render::realtime {
       /* The render engine.
        * This is different from the game engine.
        * While the game engine is responsible for coordinating the game,
        * it is the responsibility of the render engine to manage the rendering pipeline.
        **/
        class Engine2D : public Engine {
        public:
            /**
             **/
            Engine2D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry& registry);

            /**
             **/
            bool initialize(const boost::shared_ptr<Window2D>& window);

            /**
             **/
            boost::shared_ptr<Texture2DCache> textureCache();

            /**
             **/
            void renderFrame();

            /**
             **/
            boost::shared_ptr<Context2D> context();

            /**
             **/
            boost::shared_ptr<Texture2D> backBuffer();

            /**
             **/
            boost::shared_ptr<Scene2D> scene();

        private:
            boost::shared_ptr<Context2D> context_;
            boost::shared_ptr<Texture2D> backBuffer_;
            //std::list<boost::shared_ptr<Renderable>> renderables_;
            boost::shared_ptr<Scene2D> scene_;
            boost::shared_ptr<Texture2DCache> textureCache_;
    };
};  // namespace v3d::render::realtime
