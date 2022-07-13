/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/
#pragma once

#include <string>

#include "../../v3dlibs/hookah/Window.h"
#include "../../luxa/luxa/ComponentManager.h"
#include "../../luxa/luxa/menu/Menu.h"
#include "../../v3dlibs/input/KeyboardDevice.h"
#include "../../v3dlibs/input/MouseDevice.h"
#include "../../v3dlibs/gui/InputEventAdapter.h"

class PongScene;
class PongRenderer;

/**
 * The core game controller class
 */
class PongController {
 public:
    explicit PongController(const std::string & path);

    bool run();

    bool exec(const v3d::command::CommandInfo & command, const std::string & param);
    bool execUI(const v3d::command::CommandInfo & command, const std::string & param);

 protected:
     void setMenuItemDefaults(const boost::shared_ptr<Luxa::Menu> & menu);

 private:
    boost::shared_ptr<PongScene> scene_;
    boost::shared_ptr<PongRenderer> renderer_;
    boost::shared_ptr<Luxa::ComponentManager> vgui_;
    boost::shared_ptr<Hookah::Window> window_;
    boost::shared_ptr<v3d::input::KeyboardDevice> keyboard_;
    boost::shared_ptr<v3d::input::MouseDevice> mouse_;
    boost::shared_ptr<v3d::command::CommandDirectory> directory_;
    boost::shared_ptr<v3d::input::InputEventAdapter> listenerAdapter_;
};
