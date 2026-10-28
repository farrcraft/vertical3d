/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "Primitive.h"

#include <boost/shared_ptr.hpp>

namespace v3d::moya::reyes {

class Hider;

/**
 * One tile of the image, and the primitives waiting in it. The hider puts each primitive in
 * the bucket that holds the upper left corner of its raster space bound, and Buckets renders
 * them in turn once every primitive is in. A micropolygon is hidden into every sample its
 * bound touches, including samples outside its own bucket.
 */
class Bucket final {
 public:
    Bucket();
    ~Bucket();

    void addPrimitive(const boost::shared_ptr<Primitive>& primitive);
    /**
     * Render every primitive in the bucket: dice it into micropolygon grids, shade
     * them and hide them into the hider's samples. A primitive too large to dice is split
     * instead. Its pieces go back through the hider, which may add them to this bucket or
     * another one.
     * @return whether anything was split. Buckets then renders every bucket again, for pieces
     *         that landed in a bucket it had already rendered
     */
    bool render(Hider & hider, RenderContext & rc);

    size_t primitiveCount(void) const;

 private:
    std::vector<boost::shared_ptr<Primitive> > primitives_;
};
};  // namespace v3d::moya::reyes
