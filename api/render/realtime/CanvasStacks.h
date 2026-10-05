/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <deque>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime {

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

/**
 * A canvas's open clips, per ADR-0037, each already intersected with the one outside it so an
 * inner clip can only take room away.
 **/
class ClipStack final {
 public:
    /**
     * Nothing clipped.
     **/
    void reset();

    /**
     * Clip to the rectangle between two corners, in whichever order they come - a negative
     * scale swaps them.
     **/
    void push(const glm::vec2& first, const glm::vec2& second);

    /**
     * Back to what was clipped before the matching push(). Nothing at the bottom of the stack
     * means uncut.
     **/
    void pop();

    /**
     * @return whether anything is clipped
     **/
    bool clipped() const noexcept;

    /**
     * @return what is clipped to - min x, min y, max x, max y - or zero when nothing is
     **/
    glm::vec4 top() const noexcept;

 private:
    std::deque<glm::vec4> clips_;
};

};  // namespace v3d::render::realtime
