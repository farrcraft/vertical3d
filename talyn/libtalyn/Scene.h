/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/sl/Instance.h>
#include <api/type/camera/Camera.h>
#include <api/type/geometry/Ray.h>

#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::talyn {

/**
 * What every primitive is shaded with: the colour, opacity and surface shader that were
 * current when the scene made it, and the motion that carries it.
 **/
class Primitive {
 public:
    /** The colour that was current, which is SL's Cs. **/
    const glm::vec3 & colour() const;

    /**
     * The surface shader a scene named, and the space it named it in.
     *
     * Empty for a primitive built in code without one, which is then its own flat colour:
     * a scene that said nothing about shading is drawn the way it was before there was a
     * language to say it in.
     **/
    const v3d::render::offline::sl::Placed & surface() const;
    void surface(const v3d::render::offline::sl::Placed & shader);

    /** The opacity that was current, which is SL's Os. **/
    const glm::vec3 & opacity() const;
    void opacity(const glm::vec3 & value);

    /**
     * Which of the scene's motions carries the primitive, or negative for none. It is
     * stored where the motion's open end put it.
     **/
    int motion() const;

 protected:
    explicit Primitive(const glm::vec3 & colour);

 private:
    friend class Scene;
    int motion_ = -1;
    v3d::render::offline::sl::Placed surface_;
    glm::vec3 opacity_ = glm::vec3(1.0f);
    glm::vec3 colour_;
};

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

/**
 * RI's sphere: centred on its own origin, cut to the slab between two heights on its z axis
 * and swept through an angle about it, and placed in world space by the transformation that
 * was current.
 *
 * It is intersected where it is defined rather than tessellated, so its silhouette is exact
 * at any size. Its normal points out, which is RI's orientation; an inside-out sphere needs
 * `Orientation`, which this renderer does not read.
 **/
class Sphere final : public Primitive {
 public:
    /**
     * @param zmin, zmax the slab it is cut to, clamped to the radius
     * @param thetamax the sweep about z, in degrees
     * @param placement object to world, at the open end of its motion
     **/
    Sphere(float radius, float zmin, float zmax, float thetamax, const glm::mat4x4 & placement,
        const glm::vec3 & colour);

    /**
     * Where a line meets the sphere, as a parameter along it, nearest past `from`.
     *
     * The line is in world space and need not be unit length: the parameter is in its
     * units, which an affine placement keeps.
     *
     * @param point the hit in the sphere's own space, where the normal and the surface
     *        parameters are read off
     **/
    bool intersects(const glm::vec3 & origin, const glm::vec3 & direction, float from,
        float* along, glm::vec3* point) const;

    /** The outward normal at a point on the sphere in its own space, in world space. **/
    glm::vec3 normal(const glm::vec3 & point) const;
    /**
     * RI's u and v at a point in its own space: the fraction of the sweep, and the fraction of
     * the way from the bottom of the slab to the top in latitude.
     **/
    glm::vec2 parameters(const glm::vec3 & point) const;

 private:
    float radius_;
    float zmin_;
    float zmax_;
    float thetamax_;
    glm::mat4x4 toObject_;
};

/**
 * Where a ray met a primitive, and everything a shader is a function of there.
 *
 * talyn's batch is this, one point of it: the same program and the same instructions that
 * run over a grid of a hundred in moya, with a mask one bit wide.
 **/
class Hit final {
 public:
    const Primitive* primitive = nullptr;
    float distance = 0.0f;
    /** SL's P, in world space, which is talyn's current space. **/
    glm::vec3 point = glm::vec3(0.0f);
    /** SL's N and Ng: the interpolated shading normal and the triangle's plane. **/
    glm::vec3 normal = glm::vec3(0.0f);
    glm::vec3 geometric = glm::vec3(0.0f);
    /** SL's I, the direction the surface was seen along. **/
    glm::vec3 incident = glm::vec3(0.0f);
    /**
     * The surface parameters: a sphere's u and v, and a triangle's barycentric weights,
     * which stand in for them.
     **/
    float u = 0.0f;
    float v = 0.0f;
    /** SL's s and t: a sphere's u and v, and a triangle's "st" at the hit. **/
    float s = 0.0f;
    float t = 0.0f;
};

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
     * The lights shining on the scene, each with the space it was instanced in.
     *
     * A light belongs to the frame rather than to the attribute block that made it, which
     * is RI's rule and is why these are the scene's rather than a triangle's.
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
    /** The motion a primitive placed by this transformation is carried by, or -1. **/
    int motion(const v3d::render::offline::MovingTransform & placed);

    std::vector<Triangle> triangles_;
    std::vector<Sphere> spheres_;
    std::vector<v3d::render::offline::MovingTransform> motions_;
    std::vector<v3d::render::offline::sl::Placed> lights_;
    glm::vec3 background_ = glm::vec3(0.0f);
    unsigned int traceDepth_ = 2;
};

};  // namespace v3d::talyn
