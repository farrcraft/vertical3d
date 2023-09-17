/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Controller.h"

#include <functional>

#include "Renderer.h"
#include "Scene.h"
#include "game/Player.h"
#include "../../api/engine/Feature.h"

#include <boost/make_shared.hpp>


Controller::Controller(const std::string& appPath) :
    debug_(false),
    v3d::engine::Engine(appPath) {
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
    /*
    // player commands
    using namespace boost::bind::placeholders;
    directory_->add("moveUp", "voxel", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_->add("moveDown", "voxel", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_->add("moveLeft", "voxel", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_->add("moveRight", "voxel", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_->add("moveForward", "voxel", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_->add("moveBackward", "voxel", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    directory_->add("look", "voxel", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    // ui commands
    directory_->add("showGameMenu", "ui", boost::bind(&Controller::execUI, boost::ref(*this), _1, _2));
    // debug commands
    directory_->add("debug", "voxel", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
    */
    scene_ = boost::make_shared<Scene>();

    // this is actually the game controller
    // maybe we need a separate player controller class to intercept mouse events?
    mouse_->addEventListener(this, "player_controller");

    renderer_ = boost::make_shared<Renderer>(scene_, loader);

    // set the scene size according to the window canvas
    renderer_->resize(window_->width(), window_->height());

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
    }
    else if (event.name() == "moveBackward") {
        scene_->player()->move(Player::MOVE_BACKWARD);
    }
    else if (event.name() == "moveLeft") {
        scene_->player()->move(Player::MOVE_LEFT);
    }
    else if (event.name() == "moveRight") {
        scene_->player()->move(Player::MOVE_RIGHT);
    }
    else if (event.name() == "moveUp") {
        scene_->player()->move(Player::MOVE_UP);
    }
    else if (event.name() == "moveDown") {
        scene_->player()->move(Player::MOVE_DOWN);
    }
    else if (event.name() == "debug") {  // debug commands
        debug_ = !debug_;
        renderer_->debug(debug_);
    }
}

bool Controller::execUI(const v3d::command::CommandInfo & command, const std::string & param) {
    if (command.scope() != "ui") {
        return false;
    }

    if (command.name() == "showGameMenu") {
        window_->shutdown();
    }

    return false;
}

void Controller::motion(unsigned int x, unsigned int y) {
    if (!window_->active()) {
        return;
    }
    unsigned int centerX = window_->width() / 2;
    unsigned int centerY = window_->height() / 2;

    int yDelta = y - centerY;
    int xDelta = x - centerX;
    float pitch = static_cast<float>(yDelta);
    float heading = static_cast<float>(xDelta);

    scene_->player()->look(heading, pitch);
    window_->warpCursor(centerX, centerY);
}

void Controller::buttonPressed(unsigned int button) {
}

void Controller::buttonReleased(unsigned int button) {
}
