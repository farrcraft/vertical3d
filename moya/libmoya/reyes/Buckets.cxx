/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Buckets.h"

#include <api/type/geometry/AABBox.h>
#include <moya/libmoya/RenderContext.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

#include "Bucket.h"
#include "Primitive.h"

namespace v3d::moya::reyes {

Buckets::Buckets(const Sizes & sizes) : bucket_(sizes.bucket) {
    assert(bucket_.x > 0 && bucket_.y > 0);
    // rounded up, so an image that is not a whole number of buckets across still has a
    // partial bucket over its last pixels
    columns_ = (sizes.image.x + bucket_.x - 1) / bucket_.x;
    rows_ = (sizes.image.y + bucket_.y - 1) / bucket_.y;
    buckets_.assign(columns_, std::vector<Bucket>(rows_));
}

unsigned int Buckets::columns() const {
    return columns_;
}

unsigned int Buckets::rows() const {
    return rows_;
}

std::size_t Buckets::primitiveCount() const {
    std::size_t count = 0;
    for (const std::vector<Bucket> & column : buckets_) {
        for (const Bucket & bucket : column) {
            count += bucket.primitiveCount();
        }
    }
    return count;
}

std::size_t Buckets::primitiveCount(unsigned int column, unsigned int row) const {
    if (column >= columns_ || row >= rows_) {
        return 0;
    }
    return buckets_[column][row].primitiveCount();
}

void Buckets::add(const boost::shared_ptr<Primitive> & primitive, const v3d::type::geometry::AABBox & bound) {
    assert(columns_ > 0 && rows_ > 0);
    // the bound is in raster space. A primitive straddling the top or left edge has a negative
    // corner, and converting that to an unsigned bucket index is undefined behaviour
    const glm::vec3 min = bound.min();
    const float left = std::max(0.0F, min[0]);
    const float top = std::max(0.0F, min[1]);
    const unsigned int column = std::min(static_cast<unsigned int>(left) / bucket_.x, columns_ - 1);
    const unsigned int row = std::min(static_cast<unsigned int>(top) / bucket_.y, rows_ - 1);
    buckets_[column][row].addPrimitive(primitive);
}

void Buckets::render(Hider & hider, RenderContext & rc) {
    // a primitive already diced yields no further grid, so a repeated pass costs only a visit
    // to each primitive. Splitting terminates, so this loop does. Progress reaches the whole
    // frame at the end of the first pass and holds there through any pass repeated after it
    const float total = static_cast<float>(columns_) * static_cast<float>(rows_);
    float done = 0.0F;
    bool split = true;
    while (split) {
        split = false;
        for (unsigned int i = 0; i < columns_; i++) {
            for (unsigned int j = 0; j < rows_; j++) {
                split = buckets_[i][j].render(hider, rc) || split;
                done += 1.0F;
                if (!rc.report(std::min(done / total, 1.0F))) {
                    return;
                }
            }
        }
    }
}

};  // namespace v3d::moya::reyes
