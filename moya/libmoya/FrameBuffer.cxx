/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "FrameBuffer.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>

#include "ReyesPrimitive.h"

namespace v3d::moya {

FrameBuffer::FrameBuffer(unsigned int bucketSize[2], unsigned int imageSize[2]) {  //   : buckets_(boost::extents[imageSize[0] / bucketSize[0]][imageSize[1] / bucketSize[1]])
    bucketSize_[0] = bucketSize[0];
    bucketSize_[1] = bucketSize[1];
    imageSize_[0] = imageSize[0];
    imageSize_[1] = imageSize[1];

    allocate();
}

FrameBuffer::~FrameBuffer() {
}

void FrameBuffer::allocate() {
    assert(bucketSize_[0] > 0 && bucketSize_[1] > 0);

    // rounded up, not down: an image that is not a whole number of buckets across still has to
    // cover the pixels in its last, partial bucket
    bucketColumns_ = (imageSize_[0] + bucketSize_[0] - 1) / bucketSize_[0];
    bucketRows_ = (imageSize_[1] + bucketSize_[1] - 1) / bucketSize_[1];

    for (unsigned int i = 0; i < bucketColumns_; i++) {
        std::vector<Bucket> vb;
        for (unsigned int j = 0; j < bucketRows_; j++) {
            Bucket b;
            vb.push_back(b);
        }
        buckets_.push_back(vb);
    }

    planes_.reset(new v3d::render::offline::FrameBuffer(imageSize_[0], imageSize_[1], COVERAGE + 1));
    // a sample wins its pixel by being nearer than what the depth plane already holds, so an
    // untouched pixel has to start further away than anything the hider can produce
    planes_->clear(DEPTH, std::numeric_limits<float>::max());
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

void FrameBuffer::addPrimitive(const boost::shared_ptr<ReyesPrimitive> & primitive, const v3d::type::geometry::AABBox & bound) {
    assert(bucketColumns_ > 0 && bucketRows_ > 0);

    // bound should be in raster space
    glm::vec3 min = bound.min();

    // a primitive straddling the top or left edge has a negative corner, and converting that
    // to an unsigned bucket index is undefined rather than merely wrong
    float left = std::max(0.0f, min[0]);
    float top = std::max(0.0f, min[1]);

    // get the upper left corner of the bound and stick it in the corresponding bucket
    unsigned int i = static_cast<unsigned int>(left) / bucketSize_[0];
    unsigned int j = static_cast<unsigned int>(top) / bucketSize_[1];

    // likewise a corner past the right or bottom edge belongs to the last bucket on that axis
    i = std::min(i, bucketColumns_ - 1);
    j = std::min(j, bucketRows_ - 1);

    buckets_[i][j].addPrimitive(primitive);
}

void FrameBuffer::render(RenderContext & rc) {
    /*
        go through the buckets and render their contents

        A split hands its pieces back to the first pass, which buckets each by where it lands
        rather than by where the sweep has reached, so a piece can land behind it. Sweeping
        again picks those up; a primitive already diced yields no further grid, so a repeated
        sweep costs a pass over the grid and nothing else. Splitting terminates, so this does.
    */
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
