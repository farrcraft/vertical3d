/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Bucket.h"

#include "../../api/render/offline/FrameBuffer.h"

#include <vector>

namespace v3d::moya {
/*
    the screen space to be rendered is broken down into a grid of tiles (buckets).
    the framebuffer owns that grid, and the stack of float image planes the buckets
    write their samples into.
*/
class FrameBuffer {
 public:
    /**
     * The planes a render writes. The colour channels lead because the image conversion
     * takes the leading planes as the picture; the depth follows them, and is the
     * renderer's own rather than an alpha - written into one it would be a wrong picture
     * that looks like a shading fault.
     */
    enum Plane {
        RED = 0,
        GREEN = 1,
        BLUE = 2,
        DEPTH = 3
    };
    /**
     * How many of the planes are the picture.
     */
    static const unsigned int CHANNELS = 3;

    FrameBuffer(unsigned int bucketSize[2], unsigned int imageSize[2]);
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
     * The image planes the hider writes samples into, indexed by Plane.
     */
    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes(void) const;
    void addPrimitive(const boost::shared_ptr<ReyesPrimitive> & primitive, const v3d::type::AABBox & bound);
    void render(RenderContext & rc);

    typedef std::vector< std::vector<Bucket> > BucketGrid;

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
