/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "AABBox.h"

namespace v3d::type {

    AABBox::AABBox() {
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
        if (point[0] < min_[0])
            min_[0] = point[0];
        if (point[1] < min_[1])
            min_[1] = point[1];
        if (point[2] < min_[2])
            min_[2] = point[2];

        if (point[0] > max_[0])
            max_[0] = point[0];
        if (point[1] > max_[1])
            max_[1] = point[1];
        if (point[2] > max_[2])
            max_[2] = point[2];
    }

};  // namespace v3d::type
