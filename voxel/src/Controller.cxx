/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Controller.h"

#include <functional>
#include <string>

#include "Renderer.h"
#include "Scene.h"
#include "game/GameState.h"
#include "game/Player.h"
#include "../../api/config/Type.h"
#include "../../api/engine/Feature.h"
#include "../../api/render/realtime/Window3D.h"
#include "../../api/ui/component/menu/Menu.h"

#include <boost/make_shared.hpp>

namespace {

    /**
     * The container the escape key shows and hides, and the menu inside it. Both are named
     * by data/vgui.json, so the two have to be changed together.
     **/
    const char* const menuContainerName = "game-menu";
    const char* const menuName = "main-menu";

};  // namespace

Controller::Controller(const std::string& appPath) :
    v3d::engine::Engine(appPath),
    debug_(false) {
}


bool Controller::initialize() {
    if (!v3d::engine::Engine::initialize(static_cast<int>(
        v3d::engine::Feature::Config |
        v3d::engine::Feature::Window3D |
        v3d::engine::Feature::MouseInput |
        v3d::engine::Feature::KeyboardInput))) {
        return false;
    }

    window_->caption("Voxel");

    // hide the mouse cursor in the window
    window_->cursor(false);
    // move mouse cursor to center of window
    window_->warpCursor(window_->width() / 2, window_->height() / 2);

    vgui_ = boost::make_shared<v3d::ui::Engine>(eventEngine_, dispatcher_, logger_);
    if (config_) {
        boost::shared_ptr<v3d::asset::Json> uiConfig = config_->get(v3d::config::Type::Ui);
        if (uiConfig) {
            if (!vgui_->load(uiConfig)) {
                return false;
            }
        }
    }

    // register game commands
    dispatcher_->sink<v3d::event::Event>().connect<&Controller::handleEvent>(*this);
    // this is actually the game controller
    // maybe we need a separate player controller class to intercept mouse events?
    dispatcher_->sink<v3d::event::MouseMotion>().connect<&Controller::handleMotion>(*this);

    scene_ = boost::make_shared<Scene>();

    boost::shared_ptr<v3d::render::realtime::Window3D> win = boost::dynamic_pointer_cast<v3d::render::realtime::Window3D>(window());
    renderer_ = boost::make_shared<Renderer>(scene_, win, logger_, assetManager_, &registry_);
    renderer_->ui(vgui_);

    // set the scene size according to the window canvas
    renderer_->resize(window_->width(), window_->height());

    return true;
}

/**
 **/
bool Controller::tick(unsigned int delta) {
    if (!v3d::engine::Engine::tick(delta)) {
        return false;
    }
    if (!scene_->state()->paused()) {
        scene_->tick(delta);
    }
    renderer_->tick(delta);
    return true;
}

/**
 **/
bool Controller::render() {
    renderer_->draw();
    return true;
}

/**
 **/
bool Controller::shutdown() {
    if (renderer_) {
        // the device has to be idle before the window it presents to is destroyed
        renderer_->shutdown();
    }
    if (!v3d::engine::Engine::shutdown()) {
        return false;
    }
    return true;
}

/**
 **/
bool Controller::menuVisible() const {
    if (!vgui_) {
        return false;
    }
    boost::shared_ptr<v3d::ui::Container> container = vgui_->container(menuContainerName);
    return container && container->visible();
}

/**
 **/
void Controller::toggleMenu() {
    if (!vgui_) {
        return;
    }
    boost::shared_ptr<v3d::ui::Container> container = vgui_->container(menuContainerName);
    if (!container) {
        return;
    }
    boost::shared_ptr<v3d::ui::component::Menu> menu =
        boost::dynamic_pointer_cast<v3d::ui::component::Menu>(container->get(menuName));

    // the container is what is shown and hidden. A component is visible from the moment it
    // is built, so the menu itself is not the thing to ask
    if (!container->visible()) {
        scene_->state()->pause(true);
        container->visible(true);
        window_->cursor(true);
        return;
    }
    // going back up out of a submenu leaves the menu open - it is only closing the top
    // level that puts the player back in the world
    if (!menu || !menu->up()) {
        scene_->state()->pause(false);
        container->visible(false);
        window_->cursor(false);
        window_->warpCursor(window_->width() / 2, window_->height() / 2);
    }
}

void Controller::handleEvent(const v3d::event::Event& event) {
    if (event.context()->name() == "ui") {
        if (event.name() == "showGameMenu") {
            toggleMenu();
            return;
        }
        if (event.name() == "quit") {
            // not shutdown() - this is running inside the event loop, which would tick and
            // render one more frame against the window shutdown() had just destroyed
            quit();
            return;
        }

        if (!menuVisible()) {
            return;
        }
        boost::shared_ptr<v3d::ui::Container> container = vgui_->container(menuContainerName);
        boost::shared_ptr<v3d::ui::component::Menu> menu =
            boost::dynamic_pointer_cast<v3d::ui::component::Menu>(container->get(menuName));
        if (!menu) {
            return;
        }
        if (event.name() == "menuPrevious") {
            menu->previous();
        } else if (event.name() == "menuNext") {
            menu->next();
        } else if (event.name() == "selectMenu") {
            menu->activate();
        }
        return;
    }

    if (event.context()->name() != "voxel") {
        return;
    }

    // the debug overlay is readable whether or not the world is running
    if (event.name() == "debug") {
        debug_ = !debug_;
        renderer_->debug(debug_);
        return;
    }

    // nothing moves while the menu is up
    if (menuVisible()) {
        return;
    }

    // player commands
    if (event.name() == "moveForward") {
        scene_->player()->move(Player::MOVE_FORWARD);
    } else if (event.name() == "moveBackward") {
        scene_->player()->move(Player::MOVE_BACKWARD);
    } else if (event.name() == "moveLeft") {
        scene_->player()->move(Player::MOVE_LEFT);
    } else if (event.name() == "moveRight") {
        scene_->player()->move(Player::MOVE_RIGHT);
    } else if (event.name() == "moveUp") {
        scene_->player()->move(Player::MOVE_UP);
    } else if (event.name() == "moveDown") {
        scene_->player()->move(Player::MOVE_DOWN);
    }
}

void Controller::handleMotion(const v3d::event::MouseMotion& event) {
    if ((SDL_GetWindowFlags(window_->sdl()) & SDL_WINDOW_INPUT_FOCUS) == 0) {
        return;
    }
    // the menu owns the pointer while it is up, so it is not warped back to the centre
    if (menuVisible()) {
        return;
    }
    const int centerX = window_->width() / 2;
    const int centerY = window_->height() / 2;

    const glm::vec2 position = event.position();
    const float heading = position.x - static_cast<float>(centerX);
    const float pitch = position.y - static_cast<float>(centerY);

    scene_->player()->look(heading, pitch);
    window_->warpCursor(centerX, centerY);
}
