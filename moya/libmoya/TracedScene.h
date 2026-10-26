/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Area.h>
#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/rib/Requests.h>
#include <api/render/offline/trace/Primitive.h>
#include <api/render/offline/trace/Scene.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Polygon.h"
#include "Shading.h"

namespace v3d::moya {

namespace reyes {
class GridShader;
};  // namespace reyes

/**
 * The scene a ray meets, which every primitive is added to, in world space and before any
 * split, whichever hider draws the frame.
 **/
class TracedScene final {
 public:
    /** What a primitive is submitted under, beside its Shading. **/
    class Submission final {
     public:
        /** Object to world, which moves inside a motion block. **/
        const v3d::render::offline::MovingTransform* transform = nullptr;
        /** RiColor, for a primitive whose vertices carry none. **/
        glm::vec3 colour = glm::vec3(1.0F);
        /** The area light the primitive joins, or null. **/
        v3d::render::offline::Area* area = nullptr;
        /** World to camera, where a Shading is placed. **/
        glm::mat4x4 view = glm::mat4x4(1.0F);
        /** The lights that are on, in world space. **/
        v3d::render::offline::trace::Lights lights;
        /** What runs a displacement shader over a grid. **/
        reyes::GridShader* displacer = nullptr;
    };

    v3d::render::offline::trace::Scene & scene();
    const v3d::render::offline::trace::Scene & scene() const;

    /**
     * Add a polygon as a fan of triangles. A polygon whose state has a displacement shader is
     * diced in camera space into a grid of tracedAcross micropolygons a side, displaced, and
     * added as two triangles for each micropolygon. Its triangles join the area light.
     **/
    void add(const Polygon & poly, const Shading & state, const Submission & with);
    /** Add a sphere, which a ray meets exactly. **/
    void add(const v3d::render::offline::rib::Sphere & shape, const Shading & state, const Submission & with);

 private:
    /** The micropolygons a side a displaced polygon is diced into. **/
    static constexpr unsigned int tracedAcross = 64;
    void addDisplaced(const Polygon & poly, const Shading & state, const Submission & with);
    /** Give a primitive the state's surface, opacity and lights, in world space. **/
    static void shade(v3d::render::offline::trace::Primitive* primitive, const Shading & state, const Submission & with);

    v3d::render::offline::trace::Scene scene_;
};

};  // namespace v3d::moya
