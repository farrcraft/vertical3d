/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/ui/Clock.h>

#include <glm/vec4.hpp>

namespace v3d::ui {

/**
 * A colour easing from what was shown toward what is asked for.
 *
 * It starts whenever the colour asked for differs from the one asked for before. It starts from
 * the colour shown at that moment, so a change made partway through another eases on from where
 * that one had reached. The first colour asked for is shown at once, and a colour easing in from
 * nothing shown eases in its own colour, so nothing passes through black on its way in.
 **/
class Transition final {
 public:
    /**
     * @param target the colour asked for this frame
     * @param clock the time it is asked at
     * @param duration how long a change takes, in seconds. Zero or less shows the target at once
     * @return the colour to draw
     **/
    glm::vec4 toward(const glm::vec4& target, const Clock& clock, float duration);

    /**
     * Ease toward the colour last asked for with no alpha, so a fill fades out in its own colour
     * rather than through another one.
     **/
    glm::vec4 away(const Clock& clock, float duration);

 private:
    /**
     * @return the colour shown at a time, along the change under way
     **/
    glm::vec4 shown(float now) const noexcept;

    glm::vec4 from_{0.0F};
    glm::vec4 to_{0.0F};
    float start_ = 0.0F;
    float duration_ = 0.0F;
    bool started_ = false;
};

};  // namespace v3d::ui
