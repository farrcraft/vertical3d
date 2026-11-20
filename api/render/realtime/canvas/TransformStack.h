/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <deque>

#include <glm/mat4x4.hpp>

namespace v3d::render::realtime::canvas {

/**
 * A canvas's modelview stack. The identity at the bottom is the canvas's own, so a pop with
 * nothing pushed leaves it rather than emptying the stack.
 **/
class TransformStack final {
 public:
    /**
     **/
    TransformStack();

    /**
     * Back to the identity alone.
     **/
    void reset();

    /**
     * Push a copy of the current transform.
     **/
    void push();

    /**
     * Back to the transform push() saved.
     **/
    void pop();

    /**
     * @return the transform vertices are being written through
     **/
    glm::mat4& top() noexcept;
    const glm::mat4& top() const noexcept;

 private:
    std::deque<glm::mat4> transforms_;
};

};  // namespace v3d::render::realtime::canvas
