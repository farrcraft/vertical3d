/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../../v3dlibs/core/Logger.h"
#include "Player.h"

#include "../asset/Manager.h"
#include "../config/Bootstrap.h"
#include "../config/Config.h"
#include "../input/Engine.h"
#include "../ui/Window.h"
#include "../render/Engine.h"
#include "../system/Movement.h"

namespace odyssey::engine {

    /**
     * This is the game engine.
     * It is responsible for the main game loop
     **/
    class Engine final {
     public:
        /**
         * Initialize the engine.
         * Initialization includes only the minimal amount of work required to get
         * a window displayed on the screen.
         * 
         * @return bool
         **/
        bool initialize();

        /**
         * The game loop entry point
         * @return bool
         **/
        bool run();

        /**
         * Advance the game world time
         * @return bool
         **/
        bool tick();

        /**
         * @return bool
         **/
        bool shutdown();

     private:
        odyssey::config::Bootstrap bootstrap_;
        boost::shared_ptr<odyssey::config::Config> config_;
        boost::shared_ptr<odyssey::ui::Window> window_;
        boost::shared_ptr<v3d::core::Logger> logger_;

        boost::shared_ptr<Player> player_;

        boost::shared_ptr<odyssey::render::Engine> renderEngine_;
        boost::shared_ptr<odyssey::input::Engine> inputEngine_;

        boost::shared_ptr<odyssey::asset::Manager> assetManager_;

        entt::registry registry_;
        boost::shared_ptr<entt::dispatcher> dispatcher_;

        boost::shared_ptr<odyssey::system::Movement> movementSystem_;
    };

};  // namespace odyssey::engine
