/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Intersection.h"
#include "Pose.h"
#include "Primitive.h"

namespace v3d::render::offline::trace {

/**
 * A triangle with the normals a shader reads, in world space.
 **/
class Triangle final : public Primitive {
 public:
    /** Three values, one for each corner, in the order a, b, c. **/
    using Corners = std::array<glm::vec3, 3>;

    /**
     * A triangle whose shading normal is its plane, for a scene that gives no normals.
     **/
    Triangle(const Corners & corners, const glm::vec3 & colour);

    /**
     * A triangle carrying a shading normal per corner, from a scene's varying "N".
     *
     * The geometric normal stays its plane either way: SL's Ng and N are separate, and
     * faceforward() and calculatenormal() are defined in terms of both.
     **/
    Triangle(const Corners & corners, const glm::vec3 & colour, const Corners & normals);

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
    void st(const glm::vec2 & stA, const glm::vec2 & stB, const glm::vec2 & stC);
    /** SL's s and t at a hit, weighted as shadingNormal() weighs the normals. **/
    glm::vec2 st(float u, float v) const;

    /**
     * A colour at each corner, from a scene's varying "Cs". A triangle given none is the one
     * colour it was built with.
     **/
    void colours(const glm::vec3 & colourA, const glm::vec3 & colourB, const glm::vec3 & colourC);
    /** SL's Cs at a hit, weighted as shadingNormal() weighs the normals. **/
    glm::vec3 colour(float u, float v) const;
    using Primitive::colour;

    bool intersect(const v3d::type::geometry::Ray & ray, float from, const Pose & pose,
        Intersection* found) const override;
    void describe(const Intersection & found, Hit* hit) const override;
    bool bound(v3d::type::geometry::AABBox* box) const override;
    /** u and v are the weights of b and c, as at a hit. **/
    bool at(const glm::vec2 & parameters, Hit* hit) const override;

 private:
    glm::vec3 a_;
    glm::vec3 b_;
    glm::vec3 c_;
    glm::vec3 na_;
    glm::vec3 nb_;
    glm::vec3 nc_;
    glm::vec3 geometric_;
    glm::vec2 sta_ = glm::vec2(0.0F, 0.0F);
    glm::vec2 stb_ = glm::vec2(1.0F, 0.0F);
    glm::vec2 stc_ = glm::vec2(0.0F, 1.0F);
    bool coloured_ = false;
    glm::vec3 ca_ = glm::vec3(0.0F);
    glm::vec3 cb_ = glm::vec3(0.0F);
    glm::vec3 cc_ = glm::vec3(0.0F);
};

};  // namespace v3d::render::offline::trace
