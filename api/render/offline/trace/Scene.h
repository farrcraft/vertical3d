/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/sl/Placed.h>
#include <api/type/camera/Camera.h>
#include <api/type/geometry/Ray.h>

#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Hit.h"
#include "Sphere.h"
#include "Triangle.h"

namespace v3d::render::offline::trace {

/**
 * What a render context draws: a camera, the primitives it sees, the lights on them, and
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

    /**
     * The world to camera transformation a hit's "camera" space is, and where its E is.
     * The camera's own until a renderer that does not cast its primary rays from it - moya,
     * whose hider projects - names the one it sees through.
     **/
    void view(const glm::mat4x4 & toCamera);
    glm::mat4x4 view() const;
    glm::vec3 eye() const;

    void add(const Triangle & triangle);
    /**
     * A triangle placed by the open end of a transformation that may move. One that does
     * not is added as it stands.
     **/
    void add(const Triangle & triangle, const v3d::render::offline::MovingTransform & placed);
    const std::vector<Triangle> & triangles() const;

    /** A sphere, placed and moved the way a triangle is. **/
    void add(const Sphere & sphere, const v3d::render::offline::MovingTransform & placed);
    const std::vector<Sphere> & spheres() const;

    /**
     * The lights shining on a primitive that was given none of its own, which is how a
     * scene built in code names its lights once for everything in it.
     **/
    void add(const v3d::render::offline::sl::Placed & light);
    const std::vector<v3d::render::offline::sl::Placed> & lights() const;

    /**
     * The nearest primitive a ray meets beyond `from`, or false.
     *
     * @param from how far along the ray to start looking. A ray leaving a surface would
     *        otherwise meet the surface it left: that is the self intersection every
     *        tracer has, and it is why a shadow ray is offset rather than started at zero
     * @param time when, which places every moving primitive. A ray is taken back into the
     *        pose a moving primitive was stored in rather than the primitive moved, and what
     *        it hits is brought forward again
     **/
    bool nearest(const v3d::type::geometry::Ray & ray, float from, Hit* hit, float time = 0.0f) const;

    const glm::vec3 & background() const;
    void background(const glm::vec3 & colour);

    /**
     * How many rays deep a shader's trace() may go, which is RI's
     * `Option "trace" "maxdepth"`. A trace that would go deeper answers the background,
     * and that is what bounds two surfaces that trace into each other.
     **/
    unsigned int traceDepth() const;
    void traceDepth(unsigned int depth);

 private:
    v3d::type::camera::Camera camera_;
    bool viewNamed_ = false;
    glm::mat4x4 view_ = glm::mat4x4(1.0f);
    /** The motion a primitive placed by this transformation is carried by, or -1. **/
    int motion(const v3d::render::offline::MovingTransform & placed);

    std::vector<Triangle> triangles_;
    std::vector<Sphere> spheres_;
    std::vector<v3d::render::offline::MovingTransform> motions_;
    std::vector<v3d::render::offline::sl::Placed> lights_;
    glm::vec3 background_ = glm::vec3(0.0f);
    unsigned int traceDepth_ = 2;
};

};  // namespace v3d::render::offline::trace
