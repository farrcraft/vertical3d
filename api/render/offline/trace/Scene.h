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
 * What a ray can meet: the primitives in world space, the lights on them, and what a ray
 * that misses all of them is worth.
 *
 * A primitive is anything that can intersect a ray and describe the hit, and the scene holds
 * them as one list, so a new kind of primitive is a new class rather than a change here.
 **/
class Scene final {
 public:
    /**
     * Where every motion has carried its primitives by one time, from where they are stored,
     * and the way back. Worked out once for a time rather than for every ray, since every ray
     * of a sample looks at the same time.
     **/
    class Poses final {
     public:
        std::vector<glm::mat4x4> ahead;
        std::vector<glm::mat4x4> backward;
    };

    Scene();

    /**
     * The world to camera transformation a hit's "camera" space is, and where its E is.
     * The renderer names the camera it sees through; until it does, the two spaces agree.
     **/
    void view(const glm::mat4x4 & toCamera);
    glm::mat4x4 view() const;
    glm::vec3 eye() const;

    /**
     * A primitive placed by the open end of a transformation that may move. One that does
     * not is added as it stands.
     **/
    void add(const boost::shared_ptr<Primitive> & primitive, const v3d::render::offline::MovingTransform & placed);
    void add(const Triangle & triangle);
    void add(const Triangle & triangle, const v3d::render::offline::MovingTransform & placed);
    void add(const Sphere & sphere, const v3d::render::offline::MovingTransform & placed);

    const std::vector<boost::shared_ptr<const Primitive>> & primitives() const;

    /**
     * The primitives of one kind, in the order they were added - what a test or a tool asks
     * when it wants to know what a scene made of a request.
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
     * The lights shining on a primitive that was given none of its own, which is how a
     * scene built in code names its lights once for everything in it.
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
     * @param from how far along the ray to start looking. A ray leaving a surface would
     *        otherwise meet the surface it left: that is the self intersection every
     *        tracer has, and it is why a shadow ray is offset rather than started at zero
     * @param poses where the moving primitives are. A ray is taken back into the pose a
     *        moving primitive was stored in rather than the primitive moved, and what it
     *        hits is brought forward again
     **/
    bool nearest(const v3d::type::geometry::Ray & ray, float from, Hit* hit, const Poses & poses) const;

    /**
     * The same at a time, for a caller asking once rather than for every ray of a sample.
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
