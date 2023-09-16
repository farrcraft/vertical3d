/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

struct Direction final {
    glm::vec2 direction_;
};

struct Size final {
    float size_;
};

class Ball {
 public:
    explicit Ball(entt::registry* registry);

    void direction(const glm::vec2 & dir);
    glm::vec2 direction() const;

    glm::vec2 position() const;
    void position(const glm::vec2 & pos);

    void move();

    float size() const;
    void size(float s);

 private:
    entt::registry* registry_;
    entt::entity id_;
};
