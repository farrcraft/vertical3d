/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <glm/vec3.hpp>

namespace v3d::ecs::component {
/**
 * An RGB Color
 **/
class Color3 final {
 public:
    Color3(float red, float green, float blue) noexcept;

    /**
     * Move constructor
     **/
    Color3(Color3&& c) noexcept;

    /**
     * Default destructor
     **/
    ~Color3() noexcept = default;

    /**
     **/
    float red() const;

    /**
     **/
    float green() const;

    /**
     **/
    float blue() const;

    /**
     **/
    glm::vec3 value() const;

    /**
     **/
    void set(const glm::vec3& value);

    /**
     * Move assignment
     **/
    Color3& operator=(Color3&& c) noexcept;

 private:
    glm::vec3 color_;
};

};  // namespace v3d::ecs::component
