/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "FrameBuffer.h"

#include <moya/libmoya/reyes/Primitive.h>

#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>

namespace v3d::moya {

FrameBuffer::FrameBuffer(const Sizes & sizes) {
    bucketSize_[0] = sizes.bucket.x;
    bucketSize_[1] = sizes.bucket.y;
    imageSize_[0] = sizes.image.x;
    imageSize_[1] = sizes.image.y;

    allocate();
}

FrameBuffer::~FrameBuffer() {
}

void FrameBuffer::allocate() {
    assert(bucketSize_[0] > 0 && bucketSize_[1] > 0);

    // rounded up, so an image that is not a whole number of buckets across still has a
    // partial bucket over its last pixels
    bucketColumns_ = (imageSize_[0] + bucketSize_[0] - 1) / bucketSize_[0];
    bucketRows_ = (imageSize_[1] + bucketSize_[1] - 1) / bucketSize_[1];

    for (unsigned int i = 0; i < bucketColumns_; i++) {
        std::vector<reyes::Bucket> vb;
        for (unsigned int j = 0; j < bucketRows_; j++) {
            const reyes::Bucket bucket;
            vb.push_back(bucket);
        }
        buckets_.push_back(vb);
    }

    planes_.reset(new v3d::render::offline::FrameBuffer(imageSize_[0], imageSize_[1], COVERAGE + 1));
    // the film writes a depth only into a pixel some sample hit, so a pixel nothing hit
    // keeps this one and reads as further away than anything drawn
    planes_->clear({.plane = DEPTH, .value = std::numeric_limits<float>::max()});
}

boost::shared_ptr<v3d::render::offline::FrameBuffer> FrameBuffer::planes() const {
    return planes_;
}

unsigned int FrameBuffer::bucketColumns() const {
    return bucketColumns_;
}

unsigned int FrameBuffer::bucketRows() const {
    return bucketRows_;
}

size_t FrameBuffer::primitiveCount() const {
    size_t count = 0;
    for (unsigned int i = 0; i < bucketColumns_; i++) {
        for (unsigned int j = 0; j < bucketRows_; j++) {
            count += buckets_[i][j].primitiveCount();
        }
    }
    return count;
}

unsigned int * FrameBuffer::bucketSize() const {
    return const_cast<unsigned int*>(bucketSize_);
}

unsigned int * FrameBuffer::imageSize() const {
    return const_cast<unsigned int*>(imageSize_);
}

void FrameBuffer::addPrimitive(const boost::shared_ptr<reyes::Primitive> & primitive, const v3d::type::geometry::AABBox & bound) {
    assert(bucketColumns_ > 0 && bucketRows_ > 0);

    // the bound is in raster space
    glm::vec3 min = bound.min();

    // a primitive straddling the top or left edge has a negative corner, and converting that
    // to an unsigned bucket index is undefined behaviour
    const float left = std::max(0.0F, min[0]);
    const float top = std::max(0.0F, min[1]);

    // the primitive goes in the bucket that holds the upper left corner of its bound
    unsigned int column = static_cast<unsigned int>(left) / bucketSize_[0];
    unsigned int row = static_cast<unsigned int>(top) / bucketSize_[1];

    // a corner past the right or bottom edge belongs to the last bucket on that axis
    column = std::min(column, bucketColumns_ - 1);
    row = std::min(row, bucketRows_ - 1);

    buckets_[column][row].addPrimitive(primitive);
}

void FrameBuffer::render(RenderContext & rc) {
    // render every bucket, and render them all again while any of them split something. A
    // split piece is bucketed by where it lands, which can be a bucket already rendered. A
    // primitive already diced yields no further grid, so a repeated pass costs only a visit
    // to each primitive. Splitting terminates, so this loop does
    bool split = true;
    while (split) {
        split = false;
        for (unsigned int i = 0; i < bucketColumns_; i++) {
            for (unsigned int j = 0; j < bucketRows_; j++) {
                split = buckets_[i][j].render(rc) || split;
            }
        }
    }
}

};  // namespace v3d::moya
