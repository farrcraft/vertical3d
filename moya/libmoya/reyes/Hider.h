/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/film/FrameBuffer.h>
#include <api/type/geometry/AABBox.h>
#include <moya/libmoya/Curve.h>
#include <moya/libmoya/Hider.h>
#include <moya/libmoya/Polygon.h>

#include <cstddef>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Buckets.h"
#include "Primitive.h"
#include "Samples.h"

namespace v3d::render::offline {
class MovingTransform;
};  // namespace v3d::render::offline

namespace v3d::moya::state {
class RenderContext;
};  // namespace v3d::moya::state

namespace v3d::moya::reyes {

/**
 * The hider `Hider "hidden"` selects, and RI's default. Every primitive is bucketed, diced
 * into micropolygon grids and shaded, and each grid is hidden into the frame's samples.
 *
 * It works in two passes. add() bounds each primitive as the scene submits it, culls it, and
 * puts it in a bucket. render() at WorldEnd dices each bucket's primitives into grids, shades
 * and hides the grids, and splits a primitive too large to dice. A split piece comes back
 * through add().
 **/
class Hider final : public moya::Hider {
 public:
    bool traces() const override;
    /** The world begins: the buckets are made at the context's bucket size and resolution. **/
    void begin(state::RenderContext* context) override;
    /**
     * Bound a placed polygon in raster space and bucket it, or cull it.
     *
     * A polygon with more than four vertices is first broken into pieces of four or fewer,
     * because dicing interpolates four corners. A polygon outside the clipping planes or the
     * frustum is culled. One that is too large for a grid, or crosses the eye plane, is marked
     * for splitting. One that will be diced has its vertices moved into eye space. A polygon
     * that is not culled goes in the bucket that holds the upper left corner of its bound.
     **/
    void add(const boost::shared_ptr<Polygon> & poly, state::RenderContext* context) override;
    /**
     * Bound a curve segment in raster space and bucket it, or cull it, as a polygon is. One
     * that will be diced is moved into eye space.
     **/
    void add(const boost::shared_ptr<Curve> & curve, state::RenderContext* context) override;
    std::size_t bucketed() const override;
    /**
     * Render the buckets: every bucket's grids are hidden into a new set of samples, made from
     * the context's sampling as it stands, and the samples are then resolved into the planes.
     **/
    void render(state::RenderContext* context, v3d::render::offline::film::FrameBuffer* planes) override;

    /** The frame's samples, which a grid is hidden into. They exist only while rendering. **/
    Samples & samples();
    /** The buckets, for a test. Null until the world begins. **/
    const boost::shared_ptr<Buckets> & buckets() const;

 private:
    /** An axis aligned box, by its least and greatest corners. **/
    struct Bound {
        glm::vec3 min{0.0F};
        glm::vec3 max{0.0F};
    };

    /**
     * One of the eight corners of a box. Bit 0 of the index picks x, bit 1 picks y and bit 2
     * picks z, each from max when set and from min when clear.
     **/
    static glm::vec3 boxCorner(const glm::vec3 & min, const glm::vec3 & max, int index);

    /**
     * The axis aligned bound of a box carried through an affine matrix.
     *
     * All eight corners go through the matrix. Under a rotation any corner can hold the extreme
     * on an axis, so the two diagonal corners alone can leave part of the box outside.
     **/
    static Bound transformBound(const glm::mat4x4 & transform, const Bound & object);

    /**
     * Grow an eye space bound to cover where a moving primitive is at both ends of the shutter,
     * from its object space bound. A still primitive's bound is left as it is.
     **/
    static void sweep(const v3d::render::offline::MovingTransform & motion, const Bound & object, Bound* swept);

    /**
     * Give every vertex that brought no colour or normal of its own the primitive's, and every
     * vertex the primitive's plane as its geometric normal.
     *
     * A vertex with no "Cs" takes the primitive's colour, which the surface shader then reads as
     * Cs. The normals go the same way: Ng is the primitive's plane on every vertex, and a vertex
     * that brought no varying "N" shades with it, so a polygon that gives no normals is shaded
     * faceted.
     **/
    static void fillVertices(Polygon* poly);

    /**
     * Project the eight corners of two eye space bounds into raster space. A corner at or
     * behind the eye marks the primitive for splitting.
     *
     * @param grown the bound the primitive is culled and bucketed by
     * @param surface the bound before it was grown, whose raster bound is written to measured
     * @return the raster bound of grown, which is zero when no corner is in front of the eye
     **/
    static Bound rasterBound(const glm::mat4x4 & screen, const Bound & grown, const Bound & surface, Primitive* primitive,
        Bound* measured);

    /**
     * Mark a primitive for splitting when its raster bound is wider or taller than one grid.
     * A primitive already marked is left as it is.
     **/
    static void sizeTest(const Bound & measured, const state::RenderContext & context, Primitive* primitive);

    /**
     * Measure a placed primitive by its bound in object space: cull it, mark whether it can be
     * diced, and give its bound in raster space.
     *
     * A primitive beyond the clipping planes or outside the frustum is culled. One that crosses
     * the eye plane, or whose raster bound is wider or taller than one grid, is marked for
     * splitting.
     *
     * @param reach how far, in eye space, the primitive reaches past its bound. It grows the
     *        bound the primitive is culled and bucketed by, as a displacement bound does, and
     *        not the one it is measured for splitting by
     *
     * @return false for a primitive that is culled
     **/
    static bool measure(Primitive & primitive, const v3d::type::geometry::AABBox & objectBound, state::RenderContext* context,
        v3d::type::geometry::AABBox* raster, float reach = 0.0F);

    boost::shared_ptr<Buckets> buckets_;
    boost::shared_ptr<Samples> samples_;
};

};  // namespace v3d::moya::reyes
