/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Controller.h"

#include <api/engine/Engine.h>
#include <api/engine/Statistics.h>
#include <api/event/kind/MouseMotion.h>
#include <api/input/MouseState.h>
#include <api/render/realtime/vulkan/frame/Timings.h>
#include <api/ui/Immediate.h>
#include <api/ui/shell/GameMenu.h>
#include <api/ui/shell/StatisticsOverlay.h>
#include <api/window/Window.h>
#include <demos/voxel/src/engine/Nanoseconds.h>
#include <demos/voxel/src/game/GameState.h>
#include <demos/voxel/src/game/Player.h>

#include <string>
#include <utility>

#include "Renderer.h"
#include "Scene.h"

#include <boost/make_shared.hpp>

namespace {

/**
 * The button the immediate layer responds to, named as a binding config names it.
 **/
const char* const primaryButton = "left";

/**
 * The movement commands, which simulate() reads by whether their keys are held now.
 **/
const std::pair<const char*, Player::Movement> MOVES[] = {
    { "voxel::moveForward", Player::MOVE_FORWARD }, { "voxel::moveBackward", Player::MOVE_BACKWARD },
    { "voxel::moveLeft", Player::MOVE_LEFT }, { "voxel::moveRight", Player::MOVE_RIGHT },
    { "voxel::moveUp", Player::MOVE_UP }, { "voxel::moveDown", Player::MOVE_DOWN }
};

};  // namespace

Controller::Controller(const std::string& appPath) :
    v3d::engine::Engine(appPath),
    shell_(this, { .menu = true, .statistics = "" }),
    debug_(false) {
    for (const auto& [command, direction] : MOVES) {
        commands().addHeld(command);
    }
    // the debug overlay is readable whether or not the world is running
    commands().addPress("voxel::debug", [this]() {
        debug_ = !debug_;
        renderer_->debug(debug_);
    });
}


bool Controller::start() {
    window()->caption("Voxel");

    // mouselook reads how far the mouse moved. Relative mode reports motion even with the
    // pointer at the window's edge
    window()->relativeMouse(true);

    const v3d::ui::shell::GameMenu::Suspend pause = [this](bool suspended) { suspend(suspended); };
    if (!shell_.start(pause)) {
        return false;
    }

    motion_ = dispatcher()->sink<v3d::event::kind::MouseMotion>().connect<&Controller::handleMotion>(*this);

    scene_ = boost::make_shared<Scene>();

    const boost::shared_ptr<v3d::window::Window> win = window();
    renderer_ = boost::make_shared<Renderer>(scene_, win, logger(), &shell_);

    // give the camera's projection the window's aspect
    renderer_->resize(glm::ivec2(window()->width(), window()->height()));

    return true;
}

/**
 **/
bool Controller::tick(unsigned int delta) {
    if (!v3d::engine::Engine::tick(delta)) {
        return false;
    }
    // remesh in tick(), not simulate(), because the remeshing budget is a number of chunks
    // per frame. The "chunks" span times that work once per frame for the debug overlay
    const v3d::engine::Statistics::Scope chunks = measure("chunks");
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
        // movement follows the keys held now rather than counting presses, so a key let go
        // while the menu was up is not still moving the player when the menu closes
        for (const auto& [command, direction] : MOVES) {
            scene_->player()->move(direction, held(command));
        }
        scene_->tick(step);
    }
    return true;
}

/**
 **/
bool Controller::render() {
    v3d::ui::shell::StatisticsOverlay::Sample sample = shell_.sample();
    // the device's own times, for the passes it drew a few frames ago
    for (const v3d::render::realtime::vulkan::frame::Timings::Timing& pass : renderer_->timings()) {
        sample.spans.push_back({ "gpu " + pass.name, nanoseconds(pass.milliseconds) });
    }
    renderer_->draw(sample, tools());
    return true;
}

/**
 **/
v3d::ui::Immediate::Input Controller::tools() const {
    v3d::ui::Immediate::Input input;
    // no cursor while the menu is closed, because mouselook has the mouse
    if (!shell_.menu() || !shell_.menu()->visible()) {
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
    // the screen draws through the renderer, so it goes first
    shell_.release();
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
    // the menu needs a pointer, and leaving relative mode shows one
    window()->relativeMouse(!suspended);
}

void Controller::handleMotion(const v3d::event::kind::MouseMotion& event) {
    if (!window()->focused()) {
        return;
    }
    // the mouse does not turn the player while the menu is open
    if (shell_.menu()->visible()) {
        return;
    }
    const glm::vec2 moved = event.motion();
    scene_->player()->look(moved.x, moved.y);
}
