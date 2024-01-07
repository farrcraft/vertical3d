/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "PongEngine.h"

#include <iostream>
#include <string>

#include "PongRenderer.h"
#include "PongScene.h"

#include "../../api/engine/Feature.h"
#include "../../api/ecs/component/Position1D.h"
#include "../../api/ecs/component/Position2D.h"
#include "../../api/ecs/component/Color3.h"

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>


PongEngine::PongEngine(const std::string & path) : v3d::engine::Engine(path) {
}

bool::PongEngine::initialize() {
    if (!Engine::initialize(
        static_cast<int>(v3d::engine::Feature::Window3D |
        v3d::engine::Feature::KeyboardInput |
        v3d::engine::Feature::MouseInput |
        v3d::engine::Feature::Config))) {
        return false;
    }

    window_->caption("Pong!");

    soundEngine_ = boost::make_shared<v3d::audio::Engine>(logger_, dispatcher_);
    soundEngine_->initialize();

    vgui_ = boost::make_shared<v3d::ui::Engine>(eventEngine_, logger_);

    if (config_) {
        boost::shared_ptr<v3d::asset::Json> soundConfig = config_->get(v3d::config::Type::Sound);
        if (soundConfig) {
            soundEngine_->load(soundConfig);
        }

        boost::shared_ptr<v3d::asset::Json> uiConfig = config_->get(v3d::config::Type::Ui);
        if (uiConfig) {
            if (!vgui_->load(uiConfig)) {
                return false;
            }
        }
    }

    renderer_ = boost::make_shared<PongRenderer>(logger_, assetManager_, &registry_);
    scene_ = boost::make_shared<PongScene>(&registry_, dispatcher_);

    // register game commands
    dispatcher_->sink<v3d::event::Event>().connect<&PongEngine::handleEvent>(*this);

    /*
    // config commands
    directory_->add("setmode", "pong", boost::bind(&PongController::exec, boost::ref(*this), _1, _2));
    // app commands
    directory_->add("quit", "pong", boost::bind(&PongController::exec, boost::ref(*this), _1, _2));
    */

    // set the scene size according to the window canvas
    renderer_->resize(window_->width(), window_->height());

    // reset scene & game state
    scene_->reset();

    // register event listeners
    /*
    window_->addDrawListener(boost::bind(&PongRenderer::draw, boost::ref(renderer_), _1));
    window_->addResizeListener(boost::bind(&PongRenderer::resize, boost::ref(renderer_), _1, _2));
    window_->addTickListener(boost::bind(&PongScene::tick, boost::ref(scene_), _1));
    */

    // create ui
    /*
    vgui_ = boost::make_shared<Luxa::ComponentManager>(renderer_->fonts(), directory_);
    // load ui components (including fonts) from the config property tree
    Luxa::UILoader ui_loader;
    boost::property_tree::ptree config = ptree.get_child("config");
    ui_loader.load(config, &(*vgui_));
    */
    /*
        // register vgui event listeners
        window_->addPostDrawListener(boost::bind(&Luxa::ComponentManager::draw, boost::ref(vgui_), _1));
        window_->addResizeListener(boost::bind(&Luxa::ComponentManager::resize, boost::ref(vgui_), _1, _2));
        window_->addTickListener(boost::bind(&Luxa::ComponentManager::tick, boost::ref(vgui_), _1));

        // set default managed area to match canvas size
        vgui_->resize(window_->width(), window_->height());

        setMenuItemDefaults(boost::dynamic_pointer_cast<Luxa::Menu, Luxa::Component>(vgui_->getComponent("game-menu")));
    */
    return true;
}

/**
 **/
bool PongEngine::tick() {
    if (!v3d::engine::Engine::tick()) {
        return false;
    }
    scene_->tick();
    return true;
}

bool PongEngine::render() {
    return true;
}

/**
 **/
bool PongEngine::shutdown() {
    if (soundEngine_) {
        soundEngine_->shutdown();
    }
    if (!v3d::engine::Engine::shutdown()) {
        return false;
    }
    return true;
}
/*
void PongEngine::setMenuItemDefaults(const boost::shared_ptr<Luxa::Menu> & menu) {
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

bool PongEngine::setGameMode(const std::string_view& mode) {
    // changing game modes resets game state (including pause state) so the menu will need to be hidden
    boost::shared_ptr<Luxa::Menu> menu =
        boost::dynamic_pointer_cast<Luxa::Menu, Luxa::Component>(vgui_->getComponent("game-menu"));
    if (mode == "SP") {
        // set singleplayer mode
        scene_->state().coop(false);
        scene_->reset();
        menu->visible(false);
        return true;
    } else if (mode == "coop") {
        // set coop mode
        scene_->state().coop(true);
        scene_->reset();
        menu->visible(false);
        return true;
    } else if (mode == "MP") {
        // set network multiplayer mode
        scene_->state().coop(false);
        scene_->reset();
        menu->visible(false);
        return true;
    }
    return false;
}
*/
void PongEngine::handleEvent(const v3d::event::Event& event) {
    boost::shared_ptr<v3d::ui::Container> menuContainer = vgui_->container("game-menu");
    boost::shared_ptr<v3d::ui::component::Menu> menu = boost::dynamic_pointer_cast<v3d::ui::component::Menu>(menuContainer->get("main-menu"));
    bool vis = menu->visible();
    if (event.context()->name() == "pong") {
        // play commands
        if (event.name() == "leftPaddleDown") {
            if (!scene_->state().paused()) {
                scene_->left().down(!scene_->left().down());
            }
        } else if (event.name() == "rightPaddleUp") {
            if (!scene_->state().paused() && scene_->state().coop()) {
                scene_->right().up(!scene_->right().up());
            }
        } else if (event.name() == "rightPaddleDown") {
            if (!scene_->state().paused() && scene_->state().coop()) {
                scene_->right().down(!scene_->right().down());
            }
        } else if (event.name() == "showGameMenu") {
            if (!vis) {
                scene_->state().pause(true);
                menu->visible(true);
            } else {
                // need to check if this is the top-level menu or not
                // if this is a submenu, we just need to go back to the next menu up
                // only if we're in the top-level menu do we want to resume
                scene_->state().pause(false);
                menu->visible(false);
            }
        }
        return;
    } else if (event.context()->name() == "ui") {
        if (event.name() == "setMaxScore") {
            boost::optional<v3d::event::EventData> data = event.data();
            if (data) {
                unsigned int maxScore = std::get<int>(data.get());
                scene_->state().maxScore(maxScore);
            }
        } else if (event.name() == "setLeftPaddleUpKey") {
        } else if (event.name() == "setLeftPaddleDownKey") {
        } else if (event.name() == "setRightPaddleUpKey") {
        } else if (event.name() == "setRightPaddleDownKey") {
        } else if (event.name() == "setSingleplayerMode") {
            scene_->state().coop(false);
            scene_->reset();
        } else if (event.name() == "setCoopMode") {
            scene_->state().coop(true);
            scene_->reset();
        } else if (event.name() == "setMultiplayerMode") {
            scene_->state().coop(false);
            scene_->reset();
        } else if (event.name() == "quit") {
            shutdown();
            return;
        }

        if (event.name() == "showGameMenu") {
            if (!vis) {
                scene_->state().pause(true);
                menu->visible(true);
            } else {
                // if we're at the top-level menu and not in a submenu, make the game active again
                if (!menu->up()) {
                    scene_->state().pause(false);
                    menu->visible(false);
                }
            }
            return;
        }

        // the remaining ui commands only work when menu is visible
        if (!vis) {
            return;
        }

        if (event.name() == "menuPrevious") {  // select the previous menu item
            menu->previous();
        } else if (event.name() == "menuNext") {  // select the next menu item
            menu->next();
        } else if (event.name() == "selectMenu") {  // select the current menu item
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
        }
    }
}
/*
bool PongEngine::exec(const v3d::command::CommandInfo & command, const std::string & param) {
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
    } else if (command.name() == "quit") {
        return shutdown();
    }

    return false;
}

*/
