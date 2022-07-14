/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <map>
#include <string>

#include "ViewPort.h"
#include "Tool.h"

#include "../v3dlibs/core/Scene.h"
#include "../v3dlibs/hookah/Window.h"
#include "../v3dlibs/gui/InputEventAdapter.h"
#include "../v3dlibs/command/CommandDirectory.h"

#include "../luxa/luxa/ComponentManager.h"

#include <boost/property_tree/ptree.hpp>

class Controller {
 public:
        Controller();
        ~Controller();

        bool run();
        bool exec(const v3d::command::CommandInfo & command, const std::string & param);

 protected:
        void activate_tool(const std::string & name);
        void load_camera_profiles(const boost::property_tree::ptree & tree);

 private:
        boost::shared_ptr<Hookah::Window> window_;
        boost::shared_ptr<v3d::input::KeyboardDevice> keyboard_;
        boost::shared_ptr<v3d::input::MouseDevice> mouse_;
        boost::shared_ptr<v3d::command::CommandDirectory> directory_;
        boost::shared_ptr<v3d::input::InputEventAdapter> listenerAdapter_;

        boost::shared_ptr<v3d::core::Scene> scene_;
        boost::shared_ptr<v3d::ViewPort> view_;

        std::map<std::string, boost::shared_ptr<v3d::Tool> > tools_;
        boost::shared_ptr<v3d::Tool> activeTool_;

        boost::shared_ptr<Luxa::ComponentManager> cm_;
};
