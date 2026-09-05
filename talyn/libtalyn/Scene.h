/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "../../api/type/Camera.h"

#include <glm/vec3.hpp>

namespace v3d::talyn {

/**
 * A triangle with one flat colour, in world space.
 **/
class Triangle final {
 public:
    Triangle(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c, const glm::vec3 & colour);

    const glm::vec3 & a() const;
    const glm::vec3 & b() const;
    const glm::vec3 & c() const;
    const glm::vec3 & colour() const;

 private:
    glm::vec3 a_;
    glm::vec3 b_;
    glm::vec3 c_;
    glm::vec3 colour_;
};

/**
 * What a render context draws: a camera, the triangles it sees, and what a ray that
 * misses all of them is worth.
 **/
class Scene final {
 public:
    Scene();

    /**
     * The camera the primary rays come from.
     *
     * Its viewport size is set by the render to the framebuffer's, but the pixel
     * aspect is not: a frame that is not square needs one that matches, or the
     * picture is stretched across it.
     **/
    v3d::type::Camera & camera();
    const v3d::type::Camera & camera() const;

    void add(const Triangle & triangle);
    const std::vector<Triangle> & triangles() const;

    const glm::vec3 & background() const;
    void background(const glm::vec3 & colour);

 private:
    v3d::type::Camera camera_;
    std::vector<Triangle> triangles_;
    glm::vec3 background_ = glm::vec3(0.0f);
};

};  // namespace v3d::talyn
