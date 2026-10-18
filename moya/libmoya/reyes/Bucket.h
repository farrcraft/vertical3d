/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "Primitive.h"

#include <boost/shared_ptr.hpp>

namespace v3d::moya::reyes {
/**
 * One tile of the image, and the primitives waiting in it. RenderContext::addPolygon() puts
 * each primitive in the bucket that holds the upper left corner of its raster space bound.
 * FrameBuffer renders the buckets in turn once every primitive is in. A micropolygon is
 * hidden into every sample its bound touches, including samples outside its own bucket.
 */
class Bucket final {
 public:
    Bucket();
    ~Bucket();

    void addPrimitive(const boost::shared_ptr<Primitive>& primitive);
    /**
     * Render every primitive in the bucket: dice it into micropolygon grids, shade
     * them and hide them into the frame's samples. A primitive too large to dice is split
     * instead. Its pieces go back through RenderContext::addPolygon(), which may add them
     * to this bucket or another one.
     * @return whether anything was split. FrameBuffer then renders every bucket again, for
     *         pieces that landed in a bucket it had already rendered
     */
    bool render(RenderContext & rc);

    size_t primitiveCount(void) const;

 private:
    // never set or read, so every bucket holds 0 in both
    unsigned int left_ = 0;
    unsigned int top_ = 0;
    std::vector<boost::shared_ptr<Primitive> > primitives_;
};
};  // namespace v3d::moya::reyes
