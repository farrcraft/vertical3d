/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/sl/Placed.h>
#include <api/type/geometry/Ray.h>

#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Hit.h"
#include "Primitive.h"
#include "Sphere.h"
#include "Triangle.h"

namespace v3d::render::offline::trace {

/**
 * What a ray can meet: the primitives in world space, the lights on them, and the colour of
 * a ray that misses all of them.
 *
 * A primitive is anything that can intersect a ray and describe the hit, and the scene holds
 * them as one list, so a new kind of primitive is a new class rather than a change here.
 **/
class Scene final {
 public:
    /**
     * Where every motion has carried its primitives by one time, from where they are stored,
     * and the way back. Computed once per time rather than per ray, since every ray of a
     * sample has the same time.
     **/
    class Poses final {
     public:
        std::vector<glm::mat4x4> ahead;
        std::vector<glm::mat4x4> backward;
    };

    Scene();

    /**
     * The world to camera transformation that defines a hit's "camera" space, and the
     * position of its E. The renderer sets the camera; until it does, the two spaces are
     * the same.
     **/
    void view(const glm::mat4x4 & toCamera);
    glm::mat4x4 view() const;
    glm::vec3 eye() const;

    /**
     * A primitive placed by the reference end of a transformation that may move. One that does
     * not is added as it stands.
     **/
    void add(const boost::shared_ptr<Primitive> & primitive, const v3d::render::offline::MovingTransform & placed);
    void add(const Triangle & triangle);
    void add(const Triangle & triangle, const v3d::render::offline::MovingTransform & placed);
    void add(const Sphere & sphere, const v3d::render::offline::MovingTransform & placed);

    const std::vector<boost::shared_ptr<const Primitive>> & primitives() const;

    /**
     * The primitives of one kind, in the order they were added, for a test or a tool that
     * checks what a scene built from a request.
     **/
    template <class Kind>
    std::vector<const Kind*> all() const {
        std::vector<const Kind*> found;
        for (const boost::shared_ptr<const Primitive> & primitive : primitives_) {
            const Kind* kind = dynamic_cast<const Kind*>(primitive.get());
            if (kind != nullptr) {
                found.push_back(kind);
            }
        }
        return found;
    }

    /**
     * The lights shining on a primitive that was given none of its own. A scene built in
     * code names its lights here once for everything in it.
     **/
    void add(const v3d::render::offline::sl::Placed & light);
    const std::vector<v3d::render::offline::sl::Placed> & lights() const;

    /**
     * Where every moving primitive is at a time.
     **/
    Poses poses(float time) const;

    /**
     * The nearest primitive a ray meets beyond `from`, or false.
     *
     * @param from how far along the ray to start looking. Without it, a ray leaving a
     *        surface would meet the surface it left, so a shadow ray is offset rather
     *        than started at zero
     * @param poses where the moving primitives are. A ray is taken back into the pose a
     *        moving primitive was stored in rather than the primitive moved, and what it
     *        hits is brought forward again
     **/
    bool nearest(const v3d::type::geometry::Ray & ray, float from, Hit* hit, const Poses & poses) const;

    /**
     * The same at a time, for a caller tracing one ray rather than every ray of a sample.
     **/
    bool nearest(const v3d::type::geometry::Ray & ray, float from, Hit* hit, float time = 0.0f) const;

    const glm::vec3 & background() const;
    void background(const glm::vec3 & colour);

    /**
     * How many rays deep a shader's trace() may go, which is RI's
     * `Option "trace" "maxdepth"`. A trace that would go deeper returns the background,
     * which bounds the recursion between two surfaces that trace into each other.
     **/
    unsigned int traceDepth() const;
    void traceDepth(unsigned int depth);

 private:
    glm::mat4x4 view_ = glm::mat4x4(1.0f);
    /** The motion a primitive placed by this transformation is carried by, or -1. **/
    int motion(const v3d::render::offline::MovingTransform & placed);

    std::vector<boost::shared_ptr<const Primitive>> primitives_;
    std::vector<v3d::render::offline::MovingTransform> motions_;
    std::vector<v3d::render::offline::sl::Placed> lights_;
    glm::vec3 background_ = glm::vec3(0.0f);
    unsigned int traceDepth_ = 2;
};

};  // namespace v3d::render::offline::trace
