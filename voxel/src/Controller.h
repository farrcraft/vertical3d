/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once


#include <string>

#include "../../v3dlibs/hookah/Window.h"
#include "../../v3dlibs/input/KeyboardDevice.h"
#include "../../v3dlibs/input/MouseDevice.h"
#include "../../v3dlibs/gui/InputEventAdapter.h"
#include "../../v3dlibs/command/CommandDirectory.h"

#include <boost/shared_ptr.hpp>

class Renderer;
class Scene;

/**
 * Application controller
 */
class Controller :
    public v3d::MouseEventListener {
 public:
        /**
         * Initialize the controller
         *
         */
        explicit Controller(const std::string & path);

        /**
         * Run the application
         */
        bool run();

        bool exec(const v3d::CommandInfo & command, const std::string & param);
        bool execUI(const v3d::CommandInfo & command, const std::string & param);

        // mouse event listener overrides
        void motion(unsigned int x, unsigned int y);
        void buttonPressed(unsigned int button);
        void buttonReleased(unsigned int button);

        void notify(const v3D::EventInfo & e);

 private:
        boost::shared_ptr<Hookah::Window> window_;

        boost::shared_ptr<v3d::KeyboardDevice> keyboard_;
        boost::shared_ptr<v3d::MouseDevice> mouse_;

        boost::shared_ptr<v3d::CommandDirectory> directory_;
        boost::shared_ptr<v3d::InputEventAdapter> listenerAdapter_;

        boost::shared_ptr<Renderer> renderer_;
        boost::shared_ptr<Scene> scene_;

        std::string path_;

        bool debug_;
};
