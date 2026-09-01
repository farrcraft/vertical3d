/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once


#include <string>

#include "../../api/engine/Engine.h"
#include "../../api/event/Event.h"
#include "../../api/event/MouseMotion.h"

#include <boost/shared_ptr.hpp>

class Renderer;
class Scene;


/**
 * Application controller
 */
class Controller final : public v3d::engine::Engine {
 public:
        /**
         * Initialize the controller
         *
         */
        explicit Controller(const std::string& path);

        /**
          * Initialize the engine.
          * Initialization includes only the minimal amount of work required to get
          * a window displayed on the screen.
          *
          * @return bool
          **/
        bool initialize();

        /**
         * Advance the game world time
         * @return bool
         **/
        bool tick(unsigned int delta);

        /**
         * Draw the current frame
         * @return bool
         **/
        bool render();

        /**
         * @return bool
         **/
        bool shutdown();

        void handleEvent(const v3d::event::Event& event);

        /**
         * Steer the player with the cursor.
         * The cursor is warped back to the centre of the window after each move, so the
         * offset from the centre is the amount to turn by.
         **/
        void handleMotion(const v3d::event::MouseMotion& event);

 private:
        boost::shared_ptr<Scene> scene_;
        boost::shared_ptr<Renderer> renderer_;
        bool debug_;
};
