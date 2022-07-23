/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Context.h"
#include "Operation.h"
#include "Renderable.h"
#include "Scene.h"
#include "TextureCache.h"

#include "../../v3dlibs/core/Logger.h"
#include "../../v3dlibs/asset/Manager.h"
#include "../ui/Window.h"

namespace odyssey::render {

    /**
     * The render engine.
     * This is different from the game engine.  While the game engine is responsible for coordinating the game,
     * it is the responsibility of the render engine to manage the rendering pipeline.
     **/
    class Engine {
    public:
        /**
         **/
        Engine(const boost::shared_ptr<v3d::core::Logger> &logger, const boost::shared_ptr<v3d::asset::Manager> &assetManager);
        
        /**
         **/
        ~Engine();

        /**
         **/
        bool initialize(const boost::shared_ptr<odyssey::ui::Window> &window);
        
        /**
         **/
        bool shutdown();

        /**
         **/
        boost::shared_ptr<v3d::asset::Manager> assetManager();
        
        /**
         **/
        boost::shared_ptr<TextureCache> textureCache();
        
        /**
         * Handle a resize event 
         **/
        void resize(int width, int height);

        /**
         **/
        void renderFrame();

        /**
         **/
        boost::shared_ptr<Context> context();

        /**
         **/
        boost::shared_ptr<Texture> backBuffer();

        /**
         **/
        boost::shared_ptr<Scene> scene();

    private:
        boost::shared_ptr <odyssey::ui::Window> window_;
        boost::shared_ptr<Context> context_;
        boost::shared_ptr<v3d::core::Logger> logger_;
        boost::shared_ptr<Texture> backBuffer_;
        boost::shared_ptr<v3d::asset::Manager> assetManager_;
        std::list<boost::shared_ptr<Renderable>> renderables_;
        boost::shared_ptr<TextureCache> textureCache_;
        boost::shared_ptr<Scene> scene_;
    };
};  // namespace odyssey::render
