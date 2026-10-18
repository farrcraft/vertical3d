/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "AABBox.h"

#include <cstddef>

#include <glm/common.hpp>
#include <glm/vector_relational.hpp>

namespace v3d::type::geometry {

namespace {

// where vertices() puts each corner of the box
constexpr std::size_t MIN_X_MAX_Y_MAX_Z = 0;
constexpr std::size_t MAX_X_MIN_Y_MIN_Z = 1;
constexpr std::size_t MAX_X_MAX_Y_MIN_Z = 2;
constexpr std::size_t MIN_X_MAX_Y_MIN_Z = 3;
constexpr std::size_t MAX_X_MIN_Y_MAX_Z = 4;
constexpr std::size_t MIN_X_MIN_Y_MAX_Z = 5;
constexpr std::size_t MIN_X_MIN_Y_MIN_Z = 6;
constexpr std::size_t MAX_X_MAX_Y_MAX_Z = 7;

};  // namespace

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

void AABBox::vertices(glm::vec3* v) const {
    // calculate the remaining vertices of the box from the two extents.
    v[MIN_X_MAX_Y_MAX_Z][0] = min_[0];
    v[MIN_X_MAX_Y_MAX_Z][1] = max_[1];
    v[MIN_X_MAX_Y_MAX_Z][2] = max_[2];

    v[MAX_X_MIN_Y_MIN_Z][0] = max_[0];
    v[MAX_X_MIN_Y_MIN_Z][1] = min_[1];
    v[MAX_X_MIN_Y_MIN_Z][2] = min_[2];

    v[MAX_X_MAX_Y_MIN_Z][0] = max_[0];
    v[MAX_X_MAX_Y_MIN_Z][1] = max_[1];
    v[MAX_X_MAX_Y_MIN_Z][2] = min_[2];

    v[MIN_X_MAX_Y_MIN_Z][0] = min_[0];
    v[MIN_X_MAX_Y_MIN_Z][1] = max_[1];
    v[MIN_X_MAX_Y_MIN_Z][2] = min_[2];

    v[MAX_X_MIN_Y_MAX_Z][0] = max_[0];
    v[MAX_X_MIN_Y_MAX_Z][1] = min_[1];
    v[MAX_X_MIN_Y_MAX_Z][2] = max_[2];

    v[MIN_X_MIN_Y_MAX_Z][0] = min_[0];
    v[MIN_X_MIN_Y_MAX_Z][1] = min_[1];
    v[MIN_X_MIN_Y_MAX_Z][2] = max_[2];

    v[MIN_X_MIN_Y_MIN_Z] = min_;
    v[MAX_X_MAX_Y_MAX_Z] = max_;
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

bool AABBox::contains(const glm::vec3& point) const {
    return glm::all(glm::lessThanEqual(min_, point)) && glm::all(glm::lessThanEqual(point, max_));
}

bool AABBox::overlaps(const AABBox& other) const {
    return glm::all(glm::lessThanEqual(min_, other.max_)) && glm::all(glm::lessThanEqual(other.min_, max_));
}

};  // namespace v3d::type::geometry
