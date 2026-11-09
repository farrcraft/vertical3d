/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <deque>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::canvas {

/**
 * A canvas's open clips, each already intersected with the one outside it so an
 * inner clip can only take room away.
 **/
class ClipStack final {
 public:
    /**
     * Nothing clipped.
     **/
    void reset();

    /**
     * Clip to the rectangle between two corners, in whichever order they come. A negative
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
     * @return the clip rectangle as min x, min y, max x, max y, or zero when nothing is
     **/
    glm::vec4 top() const noexcept;

 private:
    std::deque<glm::vec4> clips_;
};

};  // namespace v3d::render::realtime::canvas
