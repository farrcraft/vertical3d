/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CanvasStacks.h"

#include <algorithm>

namespace v3d::render::realtime {

/**
 **/
TransformStack::TransformStack() {
    reset();
}

/**
 **/
void TransformStack::reset() {
    transforms_.clear();
    transforms_.push_back(glm::mat4(1.0f));
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

/**
 **/
void ClipStack::reset() {
    clips_.clear();
}

/**
 **/
void ClipStack::push(const glm::vec2& first, const glm::vec2& second) {
    glm::vec4 rect(std::min(first.x, second.x), std::min(first.y, second.y),
        std::max(first.x, second.x), std::max(first.y, second.y));

    if (!clips_.empty()) {
        const glm::vec4& outer = clips_.back();
        rect.x = std::max(rect.x, outer.x);
        rect.y = std::max(rect.y, outer.y);
        rect.z = std::min(rect.z, outer.z);
        rect.w = std::min(rect.w, outer.w);
    }
    // two clips that miss each other leave nothing rather than an inverted rectangle,
    // which is a validation error by the time it reaches a scissor
    rect.z = std::max(rect.x, rect.z);
    rect.w = std::max(rect.y, rect.w);

    clips_.push_back(rect);
}

/**
 **/
void ClipStack::pop() {
    if (!clips_.empty()) {
        clips_.pop_back();
    }
}

/**
 **/
bool ClipStack::clipped() const noexcept {
    return !clips_.empty();
}

/**
 **/
glm::vec4 ClipStack::top() const noexcept {
    return clips_.empty() ? glm::vec4(0.0f) : clips_.back();
}

};  // namespace v3d::render::realtime
