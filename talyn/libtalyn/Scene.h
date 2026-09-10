/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/camera/Camera.h>

#include <vector>

#include <glm/vec3.hpp>

namespace v3d::talyn {

/**
 * A triangle with one flat colour and the normals a shader reads, in world space.
 **/
class Triangle final {
 public:
    /**
     * A triangle whose shading normal is its plane, which is what a scene that says nothing
     * about its normals gets.
     **/
    Triangle(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c, const glm::vec3 & colour);

    /**
     * A triangle carrying a shading normal per corner, from a scene's varying "N".
     *
     * The geometric normal stays its plane either way: SL's Ng and N are separate, and
     * faceforward() and calculatenormal() are defined in terms of both.
     **/
    Triangle(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c, const glm::vec3 & colour,
        const glm::vec3 & na, const glm::vec3 & nb, const glm::vec3 & nc);

    const glm::vec3 & a() const;
    const glm::vec3 & b() const;
    const glm::vec3 & c() const;
    const glm::vec3 & colour() const;

    /**
     * The plane the triangle lies in, wound the way its corners are - SL's Ng. Zero for a
     * triangle with no area, which names no plane.
     **/
    const glm::vec3 & geometricNormal() const;

    /**
     * The shading normal at a hit, SL's N.
     *
     * The weights are Moller-Trumbore's, as type::Ray::intersects reports them: u weighs b
     * and v weighs c, so a carries the rest.
     **/
    glm::vec3 shadingNormal(float u, float v) const;

 private:
    glm::vec3 a_;
    glm::vec3 b_;
    glm::vec3 c_;
    glm::vec3 colour_;
    glm::vec3 na_;
    glm::vec3 nb_;
    glm::vec3 nc_;
    glm::vec3 geometric_;
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
    v3d::type::camera::Camera & camera();
    const v3d::type::camera::Camera & camera() const;

    void add(const Triangle & triangle);
    const std::vector<Triangle> & triangles() const;

    const glm::vec3 & background() const;
    void background(const glm::vec3 & colour);

 private:
    v3d::type::camera::Camera camera_;
    std::vector<Triangle> triangles_;
    glm::vec3 background_ = glm::vec3(0.0f);
};

};  // namespace v3d::talyn
