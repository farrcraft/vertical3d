/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/engine/Engine.h>
#include <api/event/Event.h>
#include <api/event/kind/MouseMotion.h>
#include <api/ui/Engine.h>
#include <api/ui/Immediate.h>
#include <api/shell/Shell.h>

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
         * Called once a frame. Remeshes up to the per-frame budget of chunks.
         * @return bool
         **/
        bool tick(unsigned int delta) override;
        bool simulate(float step) override;

        /**
         * Draw the current frame
         * @return bool
         **/
        bool render() override;

        /**
         * What to hand the immediate layer this frame.
         *
         * While the game is played, relative mode hides the cursor and holds it in the window,
         * so its position means nothing. This returns the real cursor only while the menu is
         * open. Otherwise it returns a default Input, so the debug window cannot be clicked.
         **/
        v3d::ui::Immediate::Input tools() const;

        /**
         * Turn the player by how far the mouse moved, which relative mode reports in motion().
         * Motion is ignored while the window is not focused or the menu is open.
         **/
        void handleMotion(const v3d::event::kind::MouseMotion& event);

 protected:
        /**
          * Load the ui, and build the scene and the renderer.
          *
          * @return false when the ui document cannot be loaded
          **/
        bool start() override;

        /**
         * @return bool
         **/
        bool release() override;

 private:
        /**
         * Pause the world while the menu is over it, and give the pointer back.
         *
         * Relative mode is off while the menu is open. Relative mode hides the cursor, and the
         * menu needs one to be clicked.
         **/
        void suspend(bool suspended);

        boost::shared_ptr<Scene> scene_;
        boost::shared_ptr<Renderer> renderer_;
        // the ui, the menu and the screen. The debug readout shows the frame's statistics
        v3d::shell::Shell shell_;
        bool debug_;
    // disconnects this object's delegate from the dispatcher when this object is destroyed
    entt::scoped_connection motion_;
};
