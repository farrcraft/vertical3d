/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/AABBox.h>

#include <cstddef>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

#include "Bucket.h"
#include "Primitive.h"

namespace v3d::moya {
class RenderContext;
};  // namespace v3d::moya

namespace v3d::moya::reyes {

class Hider;

/**
 * The image divided into a grid of tiles, called buckets, each holding the primitives waiting
 * in it.
 **/
class Buckets final {
 public:
    /** The size of a bucket and of the whole image, in pixels. **/
    struct Sizes {
        glm::uvec2 bucket{0U};
        glm::uvec2 image{0U};
    };

    explicit Buckets(const Sizes & sizes);

    /**
     * The grid's extent. An image that is not a whole number of buckets across still gets the
     * partial bucket that covers its last few pixels.
     **/
    unsigned int columns() const;
    unsigned int rows() const;
    /** How many primitives are waiting across every bucket. **/
    std::size_t primitiveCount() const;
    /** How many primitives are waiting in one bucket. A bucket outside the grid holds none. **/
    std::size_t primitiveCount(unsigned int column, unsigned int row) const;

    /**
     * Put a primitive in the bucket that holds the upper left corner of its raster bound. A
     * corner past an edge belongs to the bucket at that edge.
     **/
    void add(const boost::shared_ptr<Primitive> & primitive, const v3d::type::geometry::AABBox & bound);
    /**
     * Render every bucket, and render them all again while any of them split something, since
     * a split piece may land in a bucket already rendered. Progress counts the buckets of one
     * pass. A stop leaves the buckets not yet rendered empty.
     **/
    void render(Hider & hider, RenderContext & rc);

 private:
    std::vector<std::vector<Bucket>> buckets_;
    glm::uvec2 bucket_;
    unsigned int columns_ = 0;
    unsigned int rows_ = 0;
};

};  // namespace v3d::moya::reyes
