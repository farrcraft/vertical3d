/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Primitive.h"

namespace v3d::render::offline::trace {

/**
 * A triangle with the normals a shader reads, in world space.
 **/
class Triangle final : public Primitive {
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

    /**
     * The texture coordinates at each corner, from a scene's varying "st". A triangle given
     * none has (0, 0), (1, 0) and (0, 1), so its s and t are its barycentric weights.
     **/
    void st(const glm::vec2 & a, const glm::vec2 & b, const glm::vec2 & c);
    /** SL's s and t at a hit, weighted as shadingNormal() weighs the normals. **/
    glm::vec2 st(float u, float v) const;

 private:
    glm::vec3 a_;
    glm::vec3 b_;
    glm::vec3 c_;
    glm::vec3 na_;
    glm::vec3 nb_;
    glm::vec3 nc_;
    glm::vec3 geometric_;
    glm::vec2 sta_ = glm::vec2(0.0f, 0.0f);
    glm::vec2 stb_ = glm::vec2(1.0f, 0.0f);
    glm::vec2 stc_ = glm::vec2(0.0f, 1.0f);
};

};  // namespace v3d::render::offline::trace
