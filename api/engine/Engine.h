/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../log/Logger.h"
#include "../asset/Manager.h"
#include "../config/Config.h"
#include "../input/Engine.h"
#include "../event/Engine.h"
#include "../render/realtime/Window.h"

#include <entt/entt.hpp>

namespace v3d::engine {

    /**
     * This is the game engine.
     * It is responsible for the main game loop
     **/
    class Engine {
     public:
        /**
         * Constructor.
         *
         * @param appPath The fully qualified base path name from which all relative
         *                paths will be derived.
         **/
        explicit Engine(const std::string& appPath);

        /**
         * Initialize the engine.
         * Initialization includes only the minimal amount of work required to get
         * a window displayed on the screen.
         *
         * @param features The set of engine features to be enabled.
         * @return bool
         **/
        bool initialize(int features);

        /**
         * The game loop entry point
         * 
         * @return bool
         **/
        bool eventLoop();

        /**
         * Advance the game world time
         * @return bool
         **/
        virtual bool tick();

        /**
         * Render the current frame.
         * This will be called after each tick within the event loop to draw the current frame
         * 
         * @return bool
         **/
        virtual bool render();

        /**
         * @return bool
         **/
        virtual bool shutdown();

     protected:
        boost::shared_ptr<v3d::log::Logger> logger_;
        boost::shared_ptr<v3d::config::Config> config_;
        boost::shared_ptr<v3d::render::realtime::Window> window_;
        boost::shared_ptr<v3d::asset::Manager> assetManager_;
        boost::shared_ptr<entt::dispatcher> dispatcher_;
        entt::registry registry_;

     private:
         bool registerEventMappings();

         std::string appPath_;
         int features_;
         boost::shared_ptr<v3d::input::Engine> inputEngine_;
         boost::shared_ptr<v3d::event::Engine> eventEngine_;
    };

};  // namespace v3d::engine
