/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "AABBox.h"

#include <glm/common.hpp>

namespace v3d::type {

// an empty box sits at the origin rather than wherever the stack left it - BRep::bound
// returns a default-constructed box for a mesh with no vertices, and its caller reads it
AABBox::AABBox() : min_(0.0f), max_(0.0f) {
}

AABBox::~AABBox() {
}

glm::vec3 AABBox::min() const {
    return min_;
}

glm::vec3 AABBox::max() const {
    return max_;
}

void AABBox::min(const glm::vec3& v) {
    min_ = v;
}

void AABBox::max(const glm::vec3& v) {
    max_ = v;
}

glm::vec3 AABBox::origin() const {
    return (max_ - min_);
}


void AABBox::vertices(glm::vec3* v) const {
    // calculate the remaining vertices of the box from the two extents.
    v[0][0] = min_[0];
    v[0][1] = max_[1];
    v[0][2] = max_[2];

    v[1][0] = max_[0];
    v[1][1] = min_[1];
    v[1][2] = min_[2];

    v[2][0] = max_[0];
    v[2][1] = max_[1];
    v[2][2] = min_[2];

    v[3][0] = min_[0];
    v[3][1] = max_[1];
    v[3][2] = min_[2];

    v[4][0] = max_[0];
    v[4][1] = min_[1];
    v[4][2] = max_[2];

    v[5][0] = min_[0];
    v[5][1] = min_[1];
    v[5][2] = max_[2];

    v[6] = min_;
    v[7] = max_;
}


// set min & max extents
void AABBox::extents(const glm::vec3& min, const glm::vec3& max) {
    min_ = min;
    max_ = max;
}

void AABBox::extend(const glm::vec3& point) {
    min_ = glm::min(min_, point);
    max_ = glm::max(max_, point);
}

};  // namespace v3d::type
