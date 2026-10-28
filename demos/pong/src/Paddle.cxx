/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Paddle.h"

#include <api/ecs/Previous.h>
#include <demos/pong/src/component/Color3.h>
#include <demos/pong/src/component/Offset.h>
#include <demos/pong/src/component/PaddleSize.h>
#include <demos/pong/src/component/Position1D.h>
#include <demos/pong/src/component/Score.h>
#include <demos/pong/src/component/Travel.h>

namespace {

// a paddle's thickness and its length along the side it moves on, in court units
constexpr float PADDLE_THICKNESS = 15.0F;
constexpr float PADDLE_LENGTH = 50.0F;

};  // namespace

Paddle::Paddle(entt::registry* registry) :
    registry_(registry) {
    id_ = registry->create();
    // create the components attached to paddle entity
    registry->emplace<Position1D>(id_, 0.0F);
    registry->emplace<Color3>(id_, glm::vec3(1.0F));
    registry->emplace<Score>(id_, 0);
    registry->emplace<Travel>(id_, false, false);
    registry->emplace<Offset>(id_, 0.0F);
    registry->emplace<PaddleSize>(id_, PADDLE_THICKNESS, PADDLE_LENGTH);
}

glm::vec3 Paddle::color() const {
    const Color3& color = registry_->get<Color3>(id_);
    return color.value;
}

void Paddle::color(const glm::vec3 & color) {
    Color3& component = registry_->get<Color3>(id_);
    component.value = color;
}

void Paddle::move(float delta) {
    Position1D& pos = registry_->get<Position1D>(id_);
    pos.value += delta;
}

void Paddle::position(const float pos) {
    Position1D& component = registry_->get<Position1D>(id_);
    component.value = pos;
}

void Paddle::offset(const float off) {
    Offset& component = registry_->get<Offset>(id_);
    component.offset_ = off;
}

float Paddle::position() const {
    const Position1D& component = registry_->get<Position1D>(id_);
    return component.value;
}

float Paddle::drawn(float alpha) const {
    return v3d::ecs::interpolated<Position1D>(*registry_, id_, alpha).value;
}

void Paddle::settle() {
    v3d::ecs::settle<Position1D>(*registry_, id_);
}

float Paddle::offset() const {
    const Offset& component = registry_->get<Offset>(id_);
    return component.offset_;
}

void Paddle::reset() {
    Score& component = registry_->get<Score>(id_);
    component.score_ = 0;
    // the travel flags are left alone, because they follow whichever key is held
}

bool Paddle::up() {
    const Travel& travel = registry_->get<Travel>(id_);
    return travel.up_;
}

bool Paddle::down() {
    const Travel& travel = registry_->get<Travel>(id_);
    return travel.down_;
}

int Paddle::score() {
    const Score& component = registry_->get<Score>(id_);
    return component.score_;
}

void Paddle::up(bool held) {
    Travel& travel = registry_->get<Travel>(id_);
    travel.up_ = held;
}

void Paddle::down(bool held) {
    Travel& travel = registry_->get<Travel>(id_);
    travel.down_ = held;
}

void Paddle::score(int s) {
    Score& component = registry_->get<Score>(id_);
    component.score_ = s;
}

float Paddle::size() const {
    const PaddleSize& paddle = registry_->get<PaddleSize>(id_);
    return paddle.size_;
}

float Paddle::length() const {
    const PaddleSize& paddle = registry_->get<PaddleSize>(id_);
    return paddle.length_;
}
