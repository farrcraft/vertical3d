/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Controller.h"

#include <functional>
#include <string>

#include "Renderer.h"
#include "Scene.h"
#include "game/Player.h"
#include "../../api/engine/Feature.h"
#include "../../api/render/realtime/Window3D.h"

#include <boost/make_shared.hpp>


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

    // register game commands
    dispatcher_->sink<v3d::event::Event>().connect<&Controller::handleEvent>(*this);
    // this is actually the game controller
    // maybe we need a separate player controller class to intercept mouse events?
    dispatcher_->sink<v3d::event::MouseMotion>().connect<&Controller::handleMotion>(*this);

    scene_ = boost::make_shared<Scene>();

    boost::shared_ptr<v3d::render::realtime::Window3D> win = boost::dynamic_pointer_cast<v3d::render::realtime::Window3D>(window());
    renderer_ = boost::make_shared<Renderer>(scene_, win, logger_, assetManager_, &registry_);

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
    scene_->tick(delta);
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
    if (!v3d::engine::Engine::shutdown()) {
        return false;
    }
    return true;
}

void Controller::handleEvent(const v3d::event::Event& event) {
    if (event.context()->name() != "voxel") {
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
    } else if (event.name() == "debug") {  // debug commands
        debug_ = !debug_;
        renderer_->debug(debug_);
    }
}

void Controller::handleMotion(const v3d::event::MouseMotion& event) {
    if ((SDL_GetWindowFlags(window_->sdl()) & SDL_WINDOW_INPUT_FOCUS) == 0) {
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
