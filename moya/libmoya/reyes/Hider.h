/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/FrameBuffer.h>
#include <moya/libmoya/Hider.h>
#include <moya/libmoya/Polygon.h>

#include <cstddef>

#include <boost/shared_ptr.hpp>

#include "Buckets.h"
#include "Samples.h"

namespace v3d::moya {
class RenderContext;
};  // namespace v3d::moya

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
    void begin(RenderContext* context) override;
    /**
     * Bound a placed polygon in raster space and bucket it, or cull it.
     *
     * A polygon with more than four vertices is first broken into pieces of four or fewer,
     * because dicing interpolates four corners. A polygon outside the clipping planes or the
     * frustum is culled. One that is too large for a grid, or crosses the eye plane, is marked
     * for splitting. One that will be diced has its vertices moved into eye space. A polygon
     * that is not culled goes in the bucket that holds the upper left corner of its bound.
     **/
    void add(const boost::shared_ptr<Polygon> & poly, RenderContext* context) override;
    std::size_t bucketed() const override;
    /**
     * Render the buckets: every bucket's grids are hidden into a new set of samples, made from
     * the context's sampling as it stands, and the samples are then resolved into the planes.
     **/
    void render(RenderContext* context, v3d::render::offline::FrameBuffer* planes) override;

    /** The frame's samples, which a grid is hidden into. They exist only while rendering. **/
    Samples & samples();
    /** The buckets, for a test. Null until the world begins. **/
    const boost::shared_ptr<Buckets> & buckets() const;

 private:
    boost::shared_ptr<Buckets> buckets_;
    boost::shared_ptr<Samples> samples_;
};

};  // namespace v3d::moya::reyes
