/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Ball.h"

#include "../../api/ecs/component/Color3.h"
#include "../../api/ecs/component/Position2D.h"

#include <cmath>


Ball::Ball(entt::registry* registry) : registry_(registry) {
    id_ = registry->create();
    // create the components attached to ball entity
    registry->emplace<v3d::ecs::component::Position2D>(id_, 0.0f, 0.0f);
    registry->emplace<Size>(id_, 1.0f);
    registry->emplace<Direction>(id_, glm::vec2(0.0f, 0.0f));
}

void Ball::direction(const glm::vec2 & dir) {
    Direction& component = registry_->get<Direction>(id_);
    component.direction_ = dir;
}

glm::vec2 Ball::direction() const {
    Direction& component = registry_->get<Direction>(id_);
    return component.direction_;
}

glm::vec2 Ball::position() const {
    v3d::ecs::component::Position2D& component = registry_->get<v3d::ecs::component::Position2D>(id_);
    return component.value();
}

void Ball::position(const glm::vec2 & pos) {
    v3d::ecs::component::Position2D& component = registry_->get<v3d::ecs::component::Position2D>(id_);
    component.set(pos);
}

void Ball::move() {
    Direction& direction = registry_->get<Direction>(id_);
    v3d::ecs::component::Position2D& position = registry_->get<v3d::ecs::component::Position2D>(id_);
    glm::vec2 lastPosition = position.value();
    position.set(lastPosition + direction.direction_);
}

float Ball::size() const {
    Size& sz = registry_->get<Size>(id_);
    return sz.size_;
}

void Ball::size(float s) {
    Size& sz = registry_->get<Size>(id_);
    sz.size_ = s;
}
