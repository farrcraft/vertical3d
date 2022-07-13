/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "PongController.h"

#include <iostream>
#include <string>

#include "PongRenderer.h"
#include "PongScene.h"

#include "../../v3dlibs/command/BindLoader.h"
#include "../../v3dlibs/hookah/Hookah.h"
#include "../../luxa/luxa/UILoader.h"
#include "../../stark/AssetLoader.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>


PongController::PongController(const std::string & path) {
    // create a new command directory object
    directory_ = boost::make_shared<v3d::command::CommandDirectory>();

    // create new app window and set caption
    window_ = Hookah::Create3DWindow(800, 600);

    // create input devices
    keyboard_ = boost::dynamic_pointer_cast<v3d::input::KeyboardDevice, v3d::input::InputDevice>(Hookah::CreateInputDevice("keyboard"));

    // register directory as an observer of input device events
    listenerAdapter_ = boost::make_shared<v3d::input::InputEventAdapter>(keyboard_, mouse_);
    listenerAdapter_->connect(directory_.get());

    // add device to window
    window_->addInputDevice("keyboard", keyboard_);

    window_->caption("Pong!");

    // load config file into a property tree
    boost::property_tree::ptree ptree;
    std::string configFile = path + std::string("config.xml");
    boost::property_tree::read_xml(configFile, ptree);

    // create the scene
    scene_ = boost::make_shared<PongScene>(ptree, path);
    // ...and the renderer
    boost::shared_ptr<AssetLoader> loader = boost::make_shared<AssetLoader>(path);
    renderer_ = boost::make_shared<PongRenderer>(scene_, loader);

    // register game commands
    // play commands
    directory_->add("leftPaddleUp", "pong", boost::bind(&PongController::exec, boost::ref(*this), _1, _2));
    directory_->add("leftPaddleDown", "pong", boost::bind(&PongController::exec, boost::ref(*this), _1, _2));
    directory_->add("rightPaddleUp", "pong", boost::bind(&PongController::exec, boost::ref(*this), _1, _2));
    directory_->add("rightPaddleDown", "pong", boost::bind(&PongController::exec, boost::ref(*this), _1, _2));
    // ui commands
    directory_->add("showGameMenu", "ui", boost::bind(&PongController::execUI, boost::ref(*this), _1, _2));
    directory_->add("menuPrevious", "ui", boost::bind(&PongController::execUI, boost::ref(*this), _1, _2));
    directory_->add("menuNext", "ui", boost::bind(&PongController::execUI, boost::ref(*this), _1, _2));
    directory_->add("selectMenu", "ui", boost::bind(&PongController::execUI, boost::ref(*this), _1, _2));
    // config commands
    directory_->add("setmode", "pong",  boost::bind(&PongController::exec, boost::ref(*this), _1, _2));
    // app commands
    directory_->add("quit", "pong",  boost::bind(&PongController::exec, boost::ref(*this), _1, _2));

    // load key binds from the property tree
    v3d::utility::load_binds(ptree, directory_.get());

    // set the scene size according to the window canvas
    renderer_->resize(window_->width(), window_->height());

    // reset scene & game state
    scene_->reset();

    // register event listeners
    window_->addDrawListener(boost::bind(&PongRenderer::draw, boost::ref(renderer_), _1));
    window_->addResizeListener(boost::bind(&PongRenderer::resize, boost::ref(renderer_), _1, _2));
    window_->addTickListener(boost::bind(&PongScene::tick, boost::ref(scene_), _1));

    // create ui
    vgui_ = boost::make_shared<Luxa::ComponentManager>(renderer_->fonts(), directory_);
    // load ui components (including fonts) from the config property tree
    Luxa::UILoader ui_loader;
    boost::property_tree::ptree config = ptree.get_child("config");
    ui_loader.load(config, &(*vgui_));
/*
    // register vgui event listeners
    window_->addPostDrawListener(boost::bind(&Luxa::ComponentManager::draw, boost::ref(vgui_), _1));
    window_->addResizeListener(boost::bind(&Luxa::ComponentManager::resize, boost::ref(vgui_), _1, _2));
    window_->addTickListener(boost::bind(&Luxa::ComponentManager::tick, boost::ref(vgui_), _1));

    // set default managed area to match canvas size
    vgui_->resize(window_->width(), window_->height());

    setMenuItemDefaults(boost::dynamic_pointer_cast<Luxa::Menu, Luxa::Component>(vgui_->getComponent("game-menu")));
*/
}

bool PongController::run() {
    return window_->run(Hookah::Window::EVENT_HANDLING_NONBLOCKING);
}

void PongController::setMenuItemDefaults(const boost::shared_ptr<Luxa::Menu> & menu) {
    for (unsigned int i = 0; i < menu->size(); i++) {
        boost::shared_ptr<Luxa::MenuItem> item = (*menu)[i];
        if (item->command() == "set_gamevar") {
            if (item->param() == "maxScore") {
                std::string label = "Rounds: ";
                label += boost::lexical_cast<std::string>(scene_->state().maxScore());
                item->label(label);
            }
        } else if (item->command() == "set_key") {
            // param contains a command name with the scope encoded
            // get the bind with the matching command from the directory
            v3d::command::Bind bind = directory_->lookup(item->param());
            std::string label = item->label();
            label += bind.event().name();
            item->label(label);
        }
        if (item->submenu()) {
            setMenuItemDefaults(item->submenu());
        }
    }
}

bool PongController::exec(const v3d::command::CommandInfo & command, const std::string & param) {
    if (command.scope() != "pong")
        return false;

    if (command.name() == "setmode") {
        // changing game modes resets game state (including pause state) so the menu will need to be hidden
        boost::shared_ptr<Luxa::Menu> menu =
            boost::dynamic_pointer_cast<Luxa::Menu, Luxa::Component>(vgui_->getComponent("game-menu"));
        if (param == "SP") {
            // set singleplayer mode
            scene_->state().coop(false);
            scene_->reset();
            menu->visible(false);
            return true;
        } else if (param == "coop") {
            // set coop mode
            scene_->state().coop(true);
            scene_->reset();
            menu->visible(false);
            return true;
        } else if (param == "MP") {
            // set network multiplayer mode
            scene_->state().coop(false);
            scene_->reset();
            menu->visible(false);
            return true;
        }
    } else if (command.name() == "set_gamevar") {
        if (param == "maxScore") {
            // _scene->state().maxScore(max_score);
        }
    } else if (command.name() == "set_key") {
    } else if (command.name() == "leftPaddleUp") {
        if (!scene_->state().paused())
            scene_->left().up(!scene_->left().up());
        return true;
    } else if (command.name() == "leftPaddleDown") {
        if (!scene_->state().paused())
            scene_->left().down(!scene_->left().down());
        return true;
    } else if (command.name() == "rightPaddleUp") {
        if (!scene_->state().paused() && scene_->state().coop())
                scene_->right().up(!scene_->right().up());
        return true;
    } else if (command.name() == "rightPaddleDown") {
        if (!scene_->state().paused() && scene_->state().coop())
            scene_->right().down(!scene_->right().down());
        return true;
    } else if (command.name() == "quit") {
        window_->shutdown();
        return true;
    }

    return false;
}

bool PongController::execUI(const v3d::command::CommandInfo & command, const std::string & param) {
    if (command.scope() != "ui")
        return false;

    boost::shared_ptr<Luxa::Menu> menu =
        boost::dynamic_pointer_cast<Luxa::Menu, Luxa::Component>(vgui_->getComponent("game-menu"));
    bool vis = menu->visible();

    if (command.name() == "showGameMenu") {
        if (!vis) {
            scene_->state().pause(true);
            menu->visible(true);
        } else {
            if (!menu->up()) {
                scene_->state().pause(false);
                menu->visible(false);
            }
        }
        return true;
    }

    // the remaining ui commands only work when menu is visible
    if (!vis)
        return false;

    if (command.name() == "menuPrevious") {  // select the previous menu item
        menu->previous();
        return true;
    } else if (command.name() == "menuNext") {  // select the next menu item
        menu->next();
        return true;
    } else if (command.name() == "selectMenu") {  // select the current menu item
        menu->activate();
        /*
        boost::shared_ptr<Luxa::MenuItem> item = menu->active();
        if (item)
        {
            if (item->submenu()) // menu item has a submenu so activate the submenu
            {
                menu->down();
            }
            else // menu item represents a command so execute the bound command
            {
                directory_.exec(v3D::CommandInfo(item->command(), item->scope()), item->param());
            }
        }
        */
        return true;
    }
    return false;
}
