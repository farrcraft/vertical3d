/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Instance.h>
#include <api/type/camera/Camera.h>
#include <api/type/geometry/Ray.h>

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

    /**
     * The surface shader a scene named, and the space it named it in.
     *
     * Empty for a triangle built in code without one, which is then its own flat colour:
     * a scene that said nothing about shading is drawn the way it was before there was a
     * language to say it in.
     **/
    const v3d::render::offline::sl::Placed & surface() const;
    void surface(const v3d::render::offline::sl::Placed & shader);

    /** The opacity that was current, which is SL's Os. **/
    const glm::vec3 & opacity() const;
    void opacity(const glm::vec3 & value);

 private:
    v3d::render::offline::sl::Placed surface_;
    glm::vec3 opacity_ = glm::vec3(1.0f);
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
 * Where a ray met a triangle, and everything a shader is a function of there.
 *
 * talyn's batch is this, one point of it: the same program and the same instructions that
 * run over a grid of a hundred in moya, with a mask one bit wide.
 **/
class Hit final {
 public:
    const Triangle* triangle = nullptr;
    float distance = 0.0f;
    /** SL's P, in world space, which is talyn's current space. **/
    glm::vec3 point = glm::vec3(0.0f);
    /** SL's N and Ng: the interpolated shading normal and the triangle's plane. **/
    glm::vec3 normal = glm::vec3(0.0f);
    glm::vec3 geometric = glm::vec3(0.0f);
    /** SL's I, the direction the surface was seen along. **/
    glm::vec3 incident = glm::vec3(0.0f);
    /**
     * The barycentric weights, which stand in for s and t until there is a real surface
     * parameterisation to read them off.
     **/
    float u = 0.0f;
    float v = 0.0f;
};

/**
 * What a render context draws: a camera, the triangles it sees, the lights on them, and
 * what a ray that misses all of them is worth.
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

    /**
     * The lights shining on the scene, each with the space it was instanced in.
     *
     * A light belongs to the frame rather than to the attribute block that made it, which
     * is RI's rule and is why these are the scene's rather than a triangle's.
     **/
    void add(const v3d::render::offline::sl::Placed & light);
    const std::vector<v3d::render::offline::sl::Placed> & lights() const;

    /**
     * The nearest triangle a ray meets beyond `from`, or false.
     *
     * @param from how far along the ray to start looking. A ray leaving a surface would
     *        otherwise meet the surface it left: that is the self intersection every
     *        tracer has, and it is why a shadow ray is offset rather than started at zero
     **/
    bool nearest(const v3d::type::geometry::Ray & ray, float from, Hit* hit) const;

    const glm::vec3 & background() const;
    void background(const glm::vec3 & colour);

 private:
    v3d::type::camera::Camera camera_;
    std::vector<Triangle> triangles_;
    std::vector<v3d::render::offline::sl::Placed> lights_;
    glm::vec3 background_ = glm::vec3(0.0f);
};

};  // namespace v3d::talyn
