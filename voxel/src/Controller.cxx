/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Controller.h"

#include <api/config/Type.h>
#include <api/engine/Feature.h>
#include <api/render/realtime/Window.h>
#include <voxel/src/game/GameState.h>
#include <voxel/src/game/Player.h>

#include <functional>
#include <string>

#include "Renderer.h"
#include "Scene.h"

#include <boost/make_shared.hpp>

namespace {

/**
 * The button the immediate layer answers, which is the one a binding config calls "left".
 **/
const char* const primaryButton = "left";

};  // namespace

Controller::Controller(const std::string& appPath) :
    v3d::engine::Engine(appPath),
    debug_(false) {
}


bool Controller::initialize() {
    if (!v3d::engine::Engine::initialize(static_cast<int>(
        v3d::engine::Feature::Config |
        v3d::engine::Feature::Window |
        v3d::engine::Feature::MouseInput |
        v3d::engine::Feature::KeyboardInput))) {
        return false;
    }

    window_->caption("Voxel");

    // hide the mouse cursor in the window
    v3d::render::realtime::Window::cursor(false);
    // move mouse cursor to center of window
    window_->warpCursor(window_->width() / 2, window_->height() / 2);

    vgui_ = boost::make_shared<v3d::ui::Engine>(eventEngine_, dispatcher_, logger_);
    menu_ = boost::make_shared<v3d::ui::shell::GameMenu>(vgui_, [this](bool suspended) {
        suspend(suspended);
    });
    if (config_) {
        boost::shared_ptr<v3d::asset::kind::Json> uiConfig = config_->get(v3d::config::Type::Ui);
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
    dispatcher_->sink<v3d::event::kind::MouseMotion>().connect<&Controller::handleMotion>(*this);

    scene_ = boost::make_shared<Scene>();

    boost::shared_ptr<v3d::render::realtime::Window> win = window();
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
    // the renderer's per-frame work stays here rather than moving to simulate(): remeshing
    // is a budget of chunks per frame, and the debug overlay averages how long a frame took
    renderer_->tick(delta);
    return true;
}

/**
 **/
bool Controller::simulate(float step) {
    if (!v3d::engine::Engine::simulate(step)) {
        return false;
    }
    if (!scene_->state()->paused()) {
        scene_->tick(step);
    }
    return true;
}

/**
 **/
bool Controller::render() {
    const v3d::engine::Statistics& measured = statistics();
    renderer_->draw({ measured.mean(), measured.last(), measured.steps() }, tools());
    return true;
}

/**
 **/
v3d::ui::Immediate::Input Controller::tools() const {
    v3d::ui::Immediate::Input input;
    // while the game has the mouse the pointer is warped back to the centre every frame, so
    // there is no cursor to offer - the menu going up is what hands one back
    if (!menu_ || !menu_->visible()) {
        return input;
    }
    const v3d::input::MouseState* pointer = mouse();
    if (pointer == nullptr) {
        return input;  // the app did not ask for Feature::MouseInput
    }
    input.cursor = pointer->position();
    input.down = pointer->held(primaryButton);
    input.pressed = pointer->pressed(primaryButton);
    input.released = pointer->released(primaryButton);
    input.wheel = pointer->wheel();
    return input;
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
void Controller::suspend(bool suspended) {
    scene_->state()->pause(suspended);
    v3d::render::realtime::Window::cursor(suspended);
    if (!suspended) {
        window_->warpCursor(window_->width() / 2, window_->height() / 2);
    }
}

void Controller::handleEvent(const v3d::event::Event& event) {
    if (event.context()->name() == "ui") {
        if (event.name() == "showGameMenu") {
            menu_->toggle();
            return;
        }
        if (event.name() == "quit") {
            // not shutdown() - this is running inside the event loop, which would tick and
            // render one more frame against the window shutdown() had just destroyed
            quit();
            return;
        }
        menu_->navigate(event.name());
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
    if (menu_->visible()) {
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

void Controller::handleMotion(const v3d::event::kind::MouseMotion& event) {
    if (!window_->focused()) {
        return;
    }
    // the menu owns the pointer while it is up, so it is not warped back to the centre
    if (menu_->visible()) {
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
