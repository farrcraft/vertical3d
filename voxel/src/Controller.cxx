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

#include <cstdint>
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


bool Controller::start() {
    window_->caption("Voxel");

    // mouselook reads how far the mouse moved, which relative mode reports at any edge
    window_->relativeMouse(true);

    vgui_ = boost::make_shared<v3d::ui::Engine>(eventEngine_, dispatcher_, logger_);
    menu_ = boost::make_shared<v3d::ui::shell::GameMenu>(vgui_, [this](bool suspended) {
        suspend(suspended);
    });
    const boost::json::object* ui = document(v3d::config::Type::Ui);
    if (ui && !vgui_->load(*ui)) {
        return false;
    }

    // register game commands
    events_ = dispatcher_->sink<v3d::event::Event>().connect<&Controller::handleEvent>(*this);
    // this is actually the game controller
    // maybe we need a separate player controller class to intercept mouse events?
    motion_ = dispatcher_->sink<v3d::event::kind::MouseMotion>().connect<&Controller::handleMotion>(*this);

    scene_ = boost::make_shared<Scene>();

    boost::shared_ptr<v3d::render::realtime::Window> win = window();
    renderer_ = boost::make_shared<Renderer>(scene_, win, logger_, assetManager_);
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
    const v3d::engine::Statistics::Scope chunks = statistics_.scope("chunks");
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
    v3d::ui::shell::StatisticsOverlay::Sample sample{ measured.mean(), measured.last(), measured.steps(), {} };
    for (const v3d::engine::Statistics::Row& row : measured.rows()) {
        sample.spans.push_back({ row.name, row.mean });
    }
    // the device's own times, for the passes it drew a few frames ago
    for (const v3d::render::realtime::vulkan::frame::Timings::Timing& pass : renderer_->timings()) {
        sample.spans.push_back({ "gpu " + pass.name, static_cast<std::uint64_t>(pass.milliseconds * 1.0e6) });
    }
    renderer_->draw(sample, tools());
    return true;
}

/**
 **/
v3d::ui::Immediate::Input Controller::tools() const {
    v3d::ui::Immediate::Input input;
    // no cursor while the game has the mouse - see tools() in Controller.h
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
bool Controller::release() {
    if (renderer_) {
        // the device has to be idle before the window it presents to is destroyed
        renderer_->shutdown();
    }
    return true;
}

/**
 **/
void Controller::suspend(bool suspended) {
    scene_->state()->pause(suspended);
    // the menu wants a pointer, and leaving relative mode is what shows one
    window_->relativeMouse(!suspended);
}

void Controller::handleEvent(const v3d::event::Event& event) {
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
    // the menu owns the pointer while it is up
    if (menu_->visible()) {
        return;
    }
    const glm::vec2 moved = event.motion();
    scene_->player()->look(moved.x, moved.y);
}
