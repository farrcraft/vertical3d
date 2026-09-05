/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "PongEngine.h"

#include <iostream>
#include <string>

#include "PongRenderer.h"
#include "PongScene.h"

#include "../../api/asset/Sound.h"
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
        static_cast<int>(v3d::engine::Feature::Window |
        v3d::engine::Feature::KeyboardInput |
        v3d::engine::Feature::MouseInput |
        v3d::engine::Feature::Config))) {
        return false;
    }

    window_->caption("Pong!");

    soundEngine_ = boost::make_shared<v3d::audio::Engine>(logger_, dispatcher_);
    // the return is not read: a device that will not open leaves the engine silent, and the
    // engine logs why. Every clip played against it is a false return.
    soundEngine_->initialize();

    vgui_ = boost::make_shared<v3d::ui::Engine>(eventEngine_, dispatcher_, logger_);

    if (config_) {
        boost::shared_ptr<v3d::asset::Json> soundConfig = config_->get(v3d::config::Type::Sound);
        if (soundConfig) {
            // a clip is an asset like any other, so the file the config names is resolved
            // against the manager's path rather than the working directory
            soundEngine_->load(soundConfig,
                [this](const std::string& source) -> boost::shared_ptr<v3d::audio::AudioClip> {
                    boost::shared_ptr<v3d::asset::Sound> asset = boost::dynamic_pointer_cast<v3d::asset::Sound>(
                        assetManager_->load(source, v3d::asset::Type::AudioWav));
                    if (!asset) {
                        return boost::shared_ptr<v3d::audio::AudioClip>();
                    }
                    return asset->clip();
                });
        }

        boost::shared_ptr<v3d::asset::Json> uiConfig = config_->get(v3d::config::Type::Ui);
        if (uiConfig) {
            if (!vgui_->load(uiConfig)) {
                return false;
            }
        }
    }
    boost::shared_ptr<v3d::render::realtime::Window> win = window();
    renderer_ = boost::make_shared<PongRenderer>(win, logger_, assetManager_, &registry_);
    scene_ = boost::make_shared<PongScene>(&registry_, dispatcher_);
    renderer_->scene(scene_);
    renderer_->ui(vgui_);

    // register game commands
    dispatcher_->sink<v3d::event::Event>().connect<&PongEngine::handleEvent>(*this);

    // set the scene size according to the window canvas
    renderer_->resize(window_->width(), window_->height());

    // reset scene & game state
    scene_->reset();

    return true;
}

/**
 **/
bool PongEngine::tick(unsigned int delta) {
    if (!v3d::engine::Engine::tick(delta)) {
        return false;
    }
    scene_->tick();
    return true;
}

bool PongEngine::render() {
    renderer_->draw();
    return true;
}

/**
 **/
bool PongEngine::shutdown() {
    if (soundEngine_) {
        soundEngine_->shutdown();
    }
    if (renderer_) {
        // the device has to be idle before the window it presents to is destroyed
        renderer_->shutdown();
    }
    if (!v3d::engine::Engine::shutdown()) {
        return false;
    }
    return true;
}
void PongEngine::handleEvent(const v3d::event::Event& event) {
    boost::shared_ptr<v3d::ui::Container> menuContainer = vgui_->container("game-menu");
    boost::shared_ptr<v3d::ui::component::Menu> menu = boost::dynamic_pointer_cast<v3d::ui::component::Menu>(menuContainer->get("main-menu"));
    // the container is what is shown and hidden. A component is visible from the moment it
    // is built, so the menu itself is not the thing to ask
    bool vis = menuContainer->visible();
    if (event.context()->name() == "pong") {
        // play commands
        // the paddle moves while its key is held, so these follow the event's edge
        bool held = (event.state() == v3d::event::State::Pressed);
        if (event.name() == "leftPaddleUp") {
            if (!scene_->state().paused()) {
                scene_->left().up(held);
            }
        } else if (event.name() == "leftPaddleDown") {
            if (!scene_->state().paused()) {
                scene_->left().down(held);
            }
        } else if (event.name() == "rightPaddleUp") {
            if (!scene_->state().paused() && scene_->state().coop()) {
                scene_->right().up(held);
            }
        } else if (event.name() == "rightPaddleDown") {
            if (!scene_->state().paused() && scene_->state().coop()) {
                scene_->right().down(held);
            }
        } else if (event.name() == "showGameMenu") {
            if (!vis) {
                scene_->state().pause(true);
                menuContainer->visible(true);
            } else {
                // going back up out of a submenu leaves the menu open - it is only closing
                // the top level that resumes the game
                if (!menu->up()) {
                    scene_->state().pause(false);
                    menuContainer->visible(false);
                }
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
            // not shutdown() - this is running inside the event loop, which would tick and
            // render one more frame against the window shutdown() had destroyed
            quit();
            return;
        }

        if (event.name() == "showGameMenu") {
            if (!vis) {
                scene_->state().pause(true);
                menuContainer->visible(true);
            } else {
                // if we're at the top-level menu and not in a submenu, make the game active again
                if (!menu->up()) {
                    scene_->state().pause(false);
                    menuContainer->visible(false);
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
        }
    }
}
