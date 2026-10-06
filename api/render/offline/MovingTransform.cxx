/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MovingTransform.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>

namespace v3d::render::offline {

namespace {

/**
 * A matrix as a translation, a rotation and a scale along each axis, with any shear folded
 * into the rotation's columns before they are normalised.
 *
 * An axis scaled to nothing leaves its column empty. With one such axis the other two still
 * fix the rotation, and the empty column is rebuilt as their cross product. With two or more,
 * the matrix says nothing about rotation, and turned is false.
 **/
class Parts {
 public:
    explicit Parts(const glm::mat4x4 & m) {
        translation = glm::vec3(m[3]);
        glm::mat3 basis(m);
        int flat = 0;
        int empty = 0;
        for (int axis = 0; axis < 3; axis++) {
            scale[axis] = glm::length(basis[axis]);
            if (scale[axis] > 0.0f) {
                basis[axis] /= scale[axis];
            } else {
                flat++;
                empty = axis;
            }
        }
        if (flat > 1) {
            turned = false;
            return;
        }
        if (flat == 1) {
            basis[empty] = glm::cross(basis[(empty + 1) % 3], basis[(empty + 2) % 3]);
        }
        // a reflection is a negative scale, kept on x so the rest is a rotation
        if (glm::determinant(basis) < 0.0f) {
            scale.x = -scale.x;
            basis[0] = -basis[0];
        }
        rotation = glm::quat_cast(basis);
    }

    glm::vec3 translation;
    glm::vec3 scale;
    glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    /** Whether the matrix fixes a rotation at all. **/
    bool turned = true;
};

};  // namespace

MovingTransform::MovingTransform() : MovingTransform(glm::mat4x4(1.0f)) {
}

MovingTransform::MovingTransform(const glm::mat4x4 & still) :
    open_(still), close_(still), baseOpen_(still), baseClose_(still) {
}

bool MovingTransform::moving() const {
    return moving_;
}

const glm::mat4x4 & MovingTransform::open() const {
    return open_;
}

const glm::mat4x4 & MovingTransform::close() const {
    return close_;
}

namespace {

/** Whether a transformation has an inverse. Written so that a NaN determinant has none. **/
bool invertible(const glm::mat4x4 & matrix) {
    return std::fabs(glm::determinant(glm::mat3(matrix))) > 1.0e-12f;
}

};  // namespace

const glm::mat4x4 & MovingTransform::reference() const {
    return invertible(open_) ? open_ : close_;
}

bool MovingTransform::placeable() const {
    return invertible(reference());
}

const glm::vec2 & MovingTransform::times() const {
    return times_;
}

glm::mat4x4 MovingTransform::at(float time) const {
    if (!moving_ || !(times_.y > times_.x)) {
        return open_;
    }
    const float u = std::clamp((time - times_.x) / (times_.y - times_.x), 0.0f, 1.0f);
    if (u <= 0.0f) {
        return open_;
    }
    if (u >= 1.0f) {
        return close_;
    }
    glm::mat4x4 result = glm::translate(glm::mat4x4(1.0f), glm::mix(openTranslation_, closeTranslation_, u));
    result = result * glm::mat4_cast(glm::slerp(openRotation_, closeRotation_, u));
    return glm::scale(result, glm::mix(openScale_, closeScale_, u));
}

void MovingTransform::settle() {
    if (!moving_) {
        return;
    }
    const Parts from(open_);
    const Parts to(close_);
    openTranslation_ = from.translation;
    openScale_ = from.scale;
    // an end flattened on two axes or more has no rotation of its own, so it turns as the
    // other end does
    openRotation_ = from.turned ? from.rotation : to.rotation;
    closeTranslation_ = to.translation;
    closeScale_ = to.scale;
    closeRotation_ = to.turned ? to.rotation : from.rotation;
}

MovingTransform MovingTransform::after(const glm::mat4x4 & matrix) const {
    MovingTransform result(*this);
    result.open_ = open_ * matrix;
    result.close_ = close_ * matrix;
    result.settle();
    return result;
}

MovingTransform MovingTransform::before(const glm::mat4x4 & matrix) const {
    MovingTransform result(*this);
    result.open_ = matrix * open_;
    result.close_ = matrix * close_;
    result.settle();
    return result;
}

void MovingTransform::begin(const std::vector<float> & times) {
    inBlock_ = true;
    blockTimes_ = times;
    baseOpen_ = open_;
    baseClose_ = close_;
    ends_.clear();
}

bool MovingTransform::inBlock() const {
    return inBlock_;
}

void MovingTransform::end() {
    if (!inBlock_) {
        return;
    }
    inBlock_ = false;
    if (ends_.empty()) {
        return;
    }
    // the first request is the open end and the last the close, whatever the base was
    open_ = ends_.front();
    close_ = ends_.back();
    moving_ = ends_.size() > 1 && open_ != close_;
    if (blockTimes_.size() >= 2) {
        times_ = glm::vec2(blockTimes_.front(), blockTimes_[std::min(blockTimes_.size(), ends_.size()) - 1]);
    } else {
        moving_ = false;
    }
    settle();
}

void MovingTransform::concat(const glm::mat4x4 & matrix) {
    if (!inBlock_) {
        open_ = open_ * matrix;
        close_ = close_ * matrix;
        settle();
        return;
    }
    // the open end builds on the base's open and every later one on its close
    ends_.push_back((ends_.empty() ? baseOpen_ : baseClose_) * matrix);
}

void MovingTransform::replace(const glm::mat4x4 & matrix) {
    if (!inBlock_) {
        open_ = matrix;
        close_ = matrix;
        moving_ = false;
        return;
    }
    ends_.push_back(matrix);
}

};  // namespace v3d::render::offline
