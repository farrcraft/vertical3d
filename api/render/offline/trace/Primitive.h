/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Placed.h>
#include <api/type/geometry/Ray.h>

#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::trace {

/**
 * The lights a primitive is shaded by, each with the space it was instanced in. Shared, since
 * every primitive made between two changes of the lights has the same set.
 **/
typedef boost::shared_ptr<const std::vector<v3d::render::offline::sl::Placed> > Lights;

class Hit;

/**
 * Where a moving primitive is at the time a ray looks: the way from there back to the pose it
 * is stored in, and forward again. Both null for a primitive that does not move.
 **/
class Pose final {
 public:
    const glm::mat4x4* ahead = nullptr;
    const glm::mat4x4* backward = nullptr;
};

/**
 * Where a ray met a primitive, as far as the primitive needs to describe the hit afterwards.
 **/
class Intersection final {
 public:
    /** Along the ray, in the ray's own units. **/
    float distance = 0.0f;
    /** A triangle's barycentric weights. **/
    float u = 0.0f;
    float v = 0.0f;
    /** A sphere's point in its own space. **/
    glm::vec3 point = glm::vec3(0.0f);
};

/**
 * What every primitive is shaded with: the colour, opacity, surface shader and lights that
 * were current when the scene made it, and the motion that carries it.
 **/
class Primitive {
 public:
    virtual ~Primitive() = default;

    /**
     * Where a world space ray meets the primitive, nearest past `from`, with the primitive
     * where its pose puts it.
     **/
    virtual bool intersect(const v3d::type::geometry::Ray & ray, float from, const Pose & pose,
        Intersection* found) const = 0;

    /**
     * The surface at an intersection, in the pose the primitive is stored in: its normals and
     * its surface parameters. Where the hit is and what was hit are the scene's to fill in.
     **/
    virtual void describe(const Intersection & found, Hit* hit) const = 0;

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
     * The lights that were on when the scene made it, per ADR-0077. Null for a primitive
     * given none, which is shaded by the scene's own list.
     **/
    const Lights & lights() const;
    void lights(const Lights & lit);

    /**
     * Which of the scene's motions carries the primitive, or negative for none. It is
     * stored where the motion's open end put it.
     **/
    int motion() const;

 protected:
    explicit Primitive(const glm::vec3 & colour);
    Primitive(const Primitive &) = default;
    Primitive & operator=(const Primitive &) = default;

 private:
    friend class Scene;
    int motion_ = -1;
    v3d::render::offline::sl::Placed surface_;
    Lights lights_;
    glm::vec3 opacity_ = glm::vec3(1.0f);
    glm::vec3 colour_;
};

};  // namespace v3d::render::offline::trace
