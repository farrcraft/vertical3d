/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../../v3dlibs/hookah/Window.h"
#include "../../v3dlibs/command/CommandDirectory.h"
#include "../../v3dlibs/input/KeyboardDevice.h"
#include "../../v3dlibs/input/MouseDevice.h"
#include "../../v3dlibs/gui/InputEventAdapter.h"

class TetrisScene;
class TetrisRenderer;

class Controller {
 public:
    Controller();

    bool run();
    bool exec(const v3d::CommandInfo & command, const std::string & param);

 private:
    boost::shared_ptr<TetrisScene> scene_;
    boost::shared_ptr<TetrisRenderer> renderer_;
    boost::shared_ptr<Hookah::Window> window_;
    boost::shared_ptr<v3d::KeyboardDevice> keyboard_;
    boost::shared_ptr<v3d::MouseDevice> mouse_;
    v3d::CommandDirectory directory_;
    boost::shared_ptr<v3d::InputEventAdapter> listenerAdapter_;
};
