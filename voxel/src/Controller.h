/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once


#include <string>

#include "../../api/engine/Engine.h"
#include "../../api/event/Event.h"

#include <boost/shared_ptr.hpp>

// class Renderer;
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
        bool tick();

        /**
         * @return bool
         **/
        bool shutdown();

        void handleEvent(const v3d::event::Event& event);

        /*
        bool exec(const v3d::command::CommandInfo & command, const std::string & param);
        bool execUI(const v3d::command::CommandInfo & command, const std::string & param);

        // mouse event listener overrides
        void motion(unsigned int x, unsigned int y);
        void buttonPressed(unsigned int button);
        void buttonReleased(unsigned int button);
        */
 private:
     /*
        boost::shared_ptr<Hookah::Window> window_;

        boost::shared_ptr<v3d::input::KeyboardDevice> keyboard_;
        boost::shared_ptr<v3d::input::MouseDevice> mouse_;

        boost::shared_ptr<v3d::command::CommandDirectory> directory_;
        boost::shared_ptr<v3d::input::InputEventAdapter> listenerAdapter_;

        boost::shared_ptr<Renderer> renderer_;

        std::string path_;
            */
        boost::shared_ptr<Scene> scene_;
        bool debug_;
        entt::registry registry_;
};
