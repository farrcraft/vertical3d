/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/sl/Placed.h>
#include <api/type/geometry/Ray.h>

#include <cstddef>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Hierarchy.h"
#include "Hit.h"
#include "Intersection.h"
#include "Pose.h"
#include "Primitive.h"
#include "Sphere.h"
#include "Triangle.h"

namespace v3d::render::offline::trace {

/**
 * What a ray can meet: the primitives in world space, the lights on them, and the colour of
 * a ray that misses all of them.
 *
 * A primitive is anything that can intersect a ray and describe the hit. The scene holds
 * them as one list, so a new kind of primitive is a new class rather than a change here.
 **/
class Scene final {
 public:
    /**
     * For each motion at one time, the transform from where its primitives are stored to
     * where they are (ahead), and its inverse (backward). Computed once per time, not per
     * ray, because every ray of a sample has the same time.
     **/
    class Poses final {
     public:
        std::vector<glm::mat4x4> ahead;
        std::vector<glm::mat4x4> backward;
    };

    /**
     * The matrices from camera space into the projected spaces a shader can name. Each is the
     * identity until the renderer sets the camera.
     **/
    class Projection final {
     public:
        /** The projection, with the visible picture over [-1, 1] in x and y. **/
        glm::mat4x4 screen = glm::mat4x4(1.0F);
        /** Pixels: x to the right and y down from the upper left corner of the picture. **/
        glm::mat4x4 raster = glm::mat4x4(1.0F);
        /** Raster divided by the resolution, over [0, 1], with z over [0, 1] from near to far. **/
        glm::mat4x4 ndc = glm::mat4x4(1.0F);
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

    /** The camera's projection, which defines a hit's "screen", "raster" and "NDC" spaces. **/
    void projection(const Projection & projected);
    const Projection & projection() const;

    /**
     * A primitive whose geometry was placed by the reference() matrix of a transformation
     * that may move. A primitive whose transformation does not move belongs to no motion.
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
     * The primitives are held in a bounding volume hierarchy, which is built on the first call
     * after a primitive was added. That makes the call cheap for a ray but not safe to make
     * from two threads at once. A moving primitive's box holds it across the whole motion.
     * Two hits at one distance resolve to the primitive added first.
     *
     * @param from how far along the ray to start looking. Without it, a ray leaving a
     *        surface would meet the surface it left, so a shadow ray is offset rather
     *        than started at zero
     * @param poses where the moving primitives are. The ray is transformed back into a moving
     *        primitive's stored pose, and the primitive is not moved. The hit's normals are
     *        then transformed forward to the ray's time
     **/
    bool nearest(const v3d::type::geometry::Ray & ray, float from, Hit* hit, const Poses & poses) const;

    /**
     * The same at a time, for a caller tracing one ray rather than every ray of a sample.
     **/
    bool nearest(const v3d::type::geometry::Ray & ray, float from, Hit* hit, float time = 0.0F) const;

    /**
     * A hit's surface at other parameters, where the hit's primitive is at the poses' time:
     * what Primitive::at() gives, carried forward as nearest() carries a hit. The primitive and
     * the incident direction are the hit's. False when the primitive cannot place a point by its
     * parameters.
     **/
    static bool around(const Hit & hit, const glm::vec2 & parameters, const Poses & poses, Hit* near);

    /**
     * How wide one pixel of the camera's picture is at a point in world space, measured across
     * x in raster space. A scene with no camera has the identity for its projection, so a pixel
     * there is one unit wide.
     **/
    float footprint(const glm::vec3 & point) const;

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
    glm::mat4x4 view_ = glm::mat4x4(1.0F);
    Projection projection_;
    /** The motion a primitive placed by this transformation is carried by, or -1. **/
    int motion(const v3d::render::offline::MovingTransform & placed);

    /** Carries a hit's normals forward from its primitive's stored pose to the poses' time. **/
    static void carry(const Primitive & primitive, const Poses & poses, Hit* hit);

    /** Builds the hierarchy over every primitive with a bound, and lists those without one. **/
    void build() const;

    std::vector<boost::shared_ptr<const Primitive>> primitives_;
    // built by nearest() when it is first asked after an add, so a const query can build it
    mutable Hierarchy hierarchy_;
    mutable std::vector<std::size_t> unbounded_;
    // the primitive each item of the hierarchy is
    mutable std::vector<std::size_t> bounded_;
    mutable bool built_ = false;
    std::vector<v3d::render::offline::MovingTransform> motions_;
    std::vector<v3d::render::offline::sl::Placed> lights_;
    glm::vec3 background_ = glm::vec3(0.0F);
    unsigned int traceDepth_ = 2;
};

};  // namespace v3d::render::offline::trace
