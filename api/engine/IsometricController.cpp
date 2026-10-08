/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "IsometricController.h"

#include <string>
#include <string_view>
#include <utility>

#include "Engine.h"

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
        return (held_(positive) ? 1.0f : 0.0f) - (held_(negative) ? 1.0f : 0.0f);
    };
    const glm::vec2 pan(axis(commands_.panRight, commands_.panLeft), axis(commands_.panUp, commands_.panDown));
    if (pan.x != 0.0f || pan.y != 0.0f) {
        orbit_->pan(pan * (speeds_.pan * step));
    }
    // zooming in narrows the view, which is a negative change in its half height
    const float zoom = axis(commands_.zoomOut, commands_.zoomIn);
    if (zoom != 0.0f) {
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

};  // namespace v3d::engine
