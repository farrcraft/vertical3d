/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "FrameBuffer.h"

#include <algorithm>
#include <cassert>
#include <vector>

#include "ReyesPrimitive.h"

namespace v3d::moya {

FrameBuffer::FrameBuffer(unsigned int bucketSize[2], unsigned int imageSize[2]) {  //   : _buckets(boost::extents[imageSize[0] / bucketSize[0]][imageSize[1] / bucketSize[1]])
    _bucketSize[0] = bucketSize[0];
    _bucketSize[1] = bucketSize[1];
    _imageSize[0] = imageSize[0];
    _imageSize[1] = imageSize[1];

    allocate();
}

FrameBuffer::~FrameBuffer() {
}

void FrameBuffer::allocate() {
    assert(_bucketSize[0] > 0 && _bucketSize[1] > 0);

    // rounded up, not down: an image that is not a whole number of buckets across still has to
    // cover the pixels in its last, partial bucket
    _bucketColumns = (_imageSize[0] + _bucketSize[0] - 1) / _bucketSize[0];
    _bucketRows = (_imageSize[1] + _bucketSize[1] - 1) / _bucketSize[1];

    for (unsigned int i = 0; i < _bucketColumns; i++) {
        std::vector<Bucket> vb;
        for (unsigned int j = 0; j < _bucketRows; j++) {
            Bucket b;
            vb.push_back(b);
        }
        _buckets.push_back(vb);
    }
}

unsigned int FrameBuffer::bucketColumns() const {
    return _bucketColumns;
}

unsigned int FrameBuffer::bucketRows() const {
    return _bucketRows;
}

size_t FrameBuffer::primitiveCount() const {
    size_t count = 0;
    for (unsigned int i = 0; i < _bucketColumns; i++) {
        for (unsigned int j = 0; j < _bucketRows; j++) {
            count += _buckets[i][j].primitiveCount();
        }
    }
    return count;
}

unsigned int * FrameBuffer::bucketSize() const {
    return const_cast<unsigned int*>(_bucketSize);
}

unsigned int * FrameBuffer::imageSize() const {
    return const_cast<unsigned int*>(_imageSize);
}

void FrameBuffer::addPrimitive(const boost::shared_ptr<ReyesPrimitive> & primitive, const v3d::type::AABBox & bound) {
    assert(_bucketColumns > 0 && _bucketRows > 0);

    // bound should be in raster space
    glm::vec3 min = bound.min();

    // a primitive straddling the top or left edge has a negative corner, and converting that
    // to an unsigned bucket index is undefined rather than merely wrong
    float left = std::max(0.0f, min[0]);
    float top = std::max(0.0f, min[1]);

    // get the upper left corner of the bound and stick it in the corresponding bucket
    unsigned int i = static_cast<unsigned int>(left) / _bucketSize[0];
    unsigned int j = static_cast<unsigned int>(top) / _bucketSize[1];

    // likewise a corner past the right or bottom edge belongs to the last bucket on that axis
    i = std::min(i, _bucketColumns - 1);
    j = std::min(j, _bucketRows - 1);

    _buckets[i][j].addPrimitive(primitive);
}

void FrameBuffer::render(RenderContext & rc) {
    /*
        go through the buckets and render their contents
    */
    for (unsigned int i = 0; i < _bucketColumns; i++) {
        for (unsigned int j = 0; j < _bucketRows; j++) {
            _buckets[i][j].render(rc);
        }
    }
}

};  // namespace v3d::moya
