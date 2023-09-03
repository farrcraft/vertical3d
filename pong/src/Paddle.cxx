/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Paddle.h"

#include "../../api/ecs/component/Color3.h"
#include "../../api/ecs/component/Position1D.h"

Paddle::Paddle(entt::registry* registry) :
    registry_(registry) {
    id_ = registry->create();
    // create the components attached to paddle entity
    registry->emplace<v3d::ecs::component::Position1D>(id_, 0.0f);
    registry->emplace<v3d::ecs::component::Color3>(id_, 1.0f, 1.0f, 1.0f);
    registry->emplace<Score>(id_, 0);
    registry->emplace<Travel>(id_, false, false);
    registry->emplace<Offset>(id_, 0.0f);
    registry->emplace<PaddleSize>(id_, 15.0f, 50.0f);
}

glm::vec3 Paddle::color() const {
    v3d::ecs::component::Color3& color = registry_->get<v3d::ecs::component::Color3>(id_);
    return color.value();
}

void Paddle::color(const glm::vec3 & color) {
    v3d::ecs::component::Color3& component = registry_->get<v3d::ecs::component::Color3>(id_);
    component.set(color);
}

void Paddle::move(float delta) {
    v3d::ecs::component::Position1D& pos = registry_->get<v3d::ecs::component::Position1D>(id_);
    float lastPosition = pos.value();
    pos.set(lastPosition + delta);
}

void Paddle::position(const float pos) {
    v3d::ecs::component::Position1D& component = registry_->get<v3d::ecs::component::Position1D>(id_);
    component.set(pos);
}

void Paddle::offset(const float off) {
    Offset& component = registry_->get<Offset>(id_);
    component.offset_ = off;
}

float Paddle::position() const {
    v3d::ecs::component::Position1D& component = registry_->get<v3d::ecs::component::Position1D>(id_);
    return component.value();
}

float Paddle::offset() const {
    Offset& component = registry_->get<Offset>(id_);
    return component.offset_;
}

void Paddle::reset() {
    Score& component = registry_->get<Score>(id_);
    component.score_ = 0;
/* resetting these could glitch input
    _up = false;
    _down = false;
*/
}

bool Paddle::up() {
    Travel& travel = registry_->get<Travel>(id_);
    return travel.up_;
}

bool Paddle::down() {
    Travel& travel = registry_->get<Travel>(id_);
    return travel.down_;
}

int Paddle::score() {
    Score& component = registry_->get<Score>(id_);
    return component.score_;
}

void Paddle::up(bool k) {
    Travel& travel = registry_->get<Travel>(id_);
    travel.up_ = k;
}

void Paddle::down(bool k) {
    Travel& travel = registry_->get<Travel>(id_);
    travel.down_ = k;
}

void Paddle::score(int s) {
    Score& component = registry_->get<Score>(id_);
    component.score_ = s;
}

float Paddle::size() {
    PaddleSize& paddle = registry_->get<PaddleSize>(id_);
    return paddle.size_;
}

float Paddle::length() {
    PaddleSize& paddle = registry_->get<PaddleSize>(id_);
    return paddle.length_;
}
