/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/engine/Engine.h>
#include <api/event/Event.h>
#include <api/event/MouseMotion.h>
#include <api/ui/Engine.h>
#include <api/ui/shell/GameMenu.h>

#include <string>

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
        bool tick(unsigned int delta) override;
        bool simulate(float step) override;

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
        /**
         * Pause the world while the menu is over it, and give the pointer back.
         *
         * The cursor goes with the menu: mouselook warps the pointer to the centre every
         * frame, which a menu cannot be clicked or seen through.
         **/
        void suspend(bool suspended);

        boost::shared_ptr<Scene> scene_;
        boost::shared_ptr<Renderer> renderer_;
        boost::shared_ptr<v3d::ui::Engine> vgui_;
        boost::shared_ptr<v3d::ui::shell::GameMenu> menu_;
        bool debug_;
};
