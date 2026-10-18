/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/FrameBuffer.h>
#include <moya/libmoya/reyes/Bucket.h>

#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya {
/**
 * The image divided into a grid of tiles, called buckets, and the stack of float image
 * planes the frame's samples are filtered into. It owns both.
 */
class FrameBuffer {
 public:
    /**
     * The planes a render writes. The colour planes come first, because image() takes its
     * channels from the leading planes. DEPTH follows them and is not an alpha channel, so
     * the picture is written with CHANNELS planes and never four.
     */
    enum Plane : std::uint8_t {
        RED = 0,
        GREEN = 1,
        BLUE = 2,
        DEPTH = 3,
        /**
         * How much of the pixel was drawn into: the filtered fraction of its samples that
         * hit. An imager reads it as SL's alpha, to tell a pixel nothing was drawn into from
         * a black one.
         */
        COVERAGE = 4
    };
    /**
     * How many of the planes are the picture.
     */
    static const unsigned int CHANNELS = 3;

    /** The size of a bucket and of the whole image, in pixels. **/
    struct Sizes {
        glm::uvec2 bucket{0U};
        glm::uvec2 image{0U};
    };

    explicit FrameBuffer(const Sizes & sizes);
    ~FrameBuffer();

    unsigned int * bucketSize(void) const;
    unsigned int * imageSize(void) const;
    /**
     * The bucket grid's extent. An image that is not a whole number of buckets across
     * still gets the partial bucket that covers its last few pixels.
     */
    unsigned int bucketColumns(void) const;
    unsigned int bucketRows(void) const;
    /**
     * How many primitives are waiting across every bucket.
     */
    size_t primitiveCount(void) const;
    /**
     * The image planes the film resolves the frame's samples into, indexed by Plane.
     */
    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes(void) const;
    void addPrimitive(const boost::shared_ptr<reyes::Primitive> & primitive, const v3d::type::geometry::AABBox & bound);
    void render(RenderContext & rc);

    typedef std::vector< std::vector<reyes::Bucket> > BucketGrid;

 protected:
    void allocate(void);

 private:
    BucketGrid buckets_;
    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes_;
    unsigned int bucketSize_[2];
    unsigned int imageSize_[2];
    unsigned int bucketColumns_ = 0;
    unsigned int bucketRows_ = 0;
};

};  // namespace v3d::moya
