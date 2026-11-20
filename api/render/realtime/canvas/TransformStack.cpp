/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "TransformStack.h"

namespace v3d::render::realtime::canvas {

/**
 **/
TransformStack::TransformStack() {
    reset();
}

/**
 **/
void TransformStack::reset() {
    transforms_.clear();
    transforms_.push_back(glm::mat4(1.0F));
}

/**
 **/
void TransformStack::push() {
    transforms_.push_back(transforms_.back());
}

/**
 **/
void TransformStack::pop() {
    if (transforms_.size() > 1) {
        transforms_.pop_back();
    }
}

/**
 **/
glm::mat4& TransformStack::top() noexcept {
    return transforms_.back();
}

/**
 **/
const glm::mat4& TransformStack::top() const noexcept {
    return transforms_.back();
}

};  // namespace v3d::render::realtime::canvas
