/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

 //#include <list>

#include "Operation.h"
//#include "Renderable.h"
#include "Window.h"

#include "../../event/WindowResize.h"
#include "../../log/Logger.h"
#include "../../asset/Manager.h"

namespace v3d::render::realtime {
    /* The render engine.
     * This is different from the game engine.While the game engine is responsible for coordinating the game,
     * it is the responsibility of the render engine to manage the rendering pipeline.
     **/
    class Engine {
    public:
        /**
         **/
        Engine(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager);

        /**
         **/
        ~Engine();

        bool initialize(const boost::shared_ptr<Window>& window);

        /**
         **/
        virtual bool shutdown();

        /**
         **/
        boost::shared_ptr<v3d::asset::Manager> assetManager();

        boost::shared_ptr<Window> window();

        /**
         * Handle a resize event
         **/
        void resize(const v3d::event::WindowResize& event);

        /**
         **/
        virtual void renderFrame() = 0;

    private:
        boost::shared_ptr<v3d::log::Logger> logger_;
        boost::shared_ptr<v3d::asset::Manager> assetManager_;
        boost::shared_ptr<Window> window_;
        //std::list<boost::shared_ptr<Renderable>> renderables_;
    };
};  // namespace v3d::render::realtime
