/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "IsometricController.h"

#include <api/event/Event.h>
#include <api/event/State.h>
#include <api/type/camera/Isometric.h>

#include <string>
#include <string_view>
#include <utility>

#include "Engine.h"

#include <glm/common.hpp>
#include <glm/vec2.hpp>

namespace v3d::engine {

/**
 **/
IsometricController::IsometricController(type::camera::Isometric* orbit, const boost::shared_ptr<entt::dispatcher>& dispatcher,
    Held held, const Commands& commands, const Speeds& speeds) :
    orbit_(orbit),
    held_(std::move(held)),
    commands_(commands),
    speeds_(speeds) {
    if (dispatcher) {
        listening_ = dispatcher->sink<event::Event>().connect<&IsometricController::receive>(*this);
    }
}

/**
 **/
IsometricController::IsometricController(type::camera::Isometric* orbit, const Engine& engine, const Commands& commands,
    const Speeds& speeds) :
    IsometricController(orbit, engine.dispatcher(), [&engine](std::string_view command) { return engine.held(command); }, commands, speeds) {
}

/**
 **/
void IsometricController::simulate(float step) {
    if (orbit_ == nullptr || !held_) {
        return;
    }
    // each axis is the difference of its two commands, so holding both cancels out
    const auto axis = [this](const std::string& positive, const std::string& negative) {
        return (held_(positive) ? 1.0F : 0.0F) - (held_(negative) ? 1.0F : 0.0F);
    };
    // the cursor at an edge pans as the command for that edge would, and with a key held toward
    // the same edge it still pans at one speed, not two
    const glm::vec2 edge = edgeAxes();
    const glm::vec2 pan = glm::clamp(
        glm::vec2(axis(commands_.panRight, commands_.panLeft), axis(commands_.panUp, commands_.panDown)) + edge,
        glm::vec2(-1.0F), glm::vec2(1.0F));
    if (pan.x != 0.0F || pan.y != 0.0F) {
        orbit_->pan(pan * (speeds_.pan * step));
    }
    // zooming in narrows the view, which is a negative change in its half height
    const float zoom = axis(commands_.zoomOut, commands_.zoomIn);
    if (zoom != 0.0F) {
        orbit_->zoomBy(zoom * speeds_.zoom * step);
    }
}

/**
 **/
void IsometricController::receive(const event::Event& event) {
    if (orbit_ == nullptr || event.state() != event::State::Pressed || event.repeat()) {
        return;
    }
    const std::string command = event.str();
    if (command == commands_.rotateLeft) {
        orbit_->rotate(-1);
    } else if (command == commands_.rotateRight) {
        orbit_->rotate(1);
    }
}

/**
 **/
void IsometricController::wheel(float notches) {
    if (orbit_ == nullptr || notches == 0.0F) {
        return;
    }
    // a notch away from the player zooms in, which narrows the view's half height
    orbit_->zoomBy(-notches * speeds_.notch);
}

/**
 **/
void IsometricController::edgePan(float margin) noexcept {
    margin_ = margin > 0.0F ? margin : 0.0F;
}

/**
 **/
void IsometricController::cursor(const glm::vec2& at, const glm::vec2& viewport, bool overUi) noexcept {
    cursor_ = at;
    viewport_ = viewport;
    overUi_ = overUi;
}

/**
 **/
glm::vec2 IsometricController::edgeAxes() const noexcept {
    // a cursor outside the window, as one that has left it is, pans nothing
    const bool inside = cursor_.x >= 0.0F && cursor_.y >= 0.0F && cursor_.x < viewport_.x && cursor_.y < viewport_.y;
    if (margin_ <= 0.0F || overUi_ || !inside) {
        return glm::vec2(0.0F);
    }
    const auto toward = [this](float at, float size) {
        if (at < margin_) {
            return -1.0F;
        }
        return at >= size - margin_ ? 1.0F : 0.0F;
    };
    // the top of the window is forward, which is the positive pan, and y grows down the window
    return glm::vec2(toward(cursor_.x, viewport_.x), -toward(cursor_.y, viewport_.y));
}

};  // namespace v3d::engine
