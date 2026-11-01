/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

#include <glm/vec4.hpp>

namespace v3d::ui {

/**
 * The time a ui is drawn at: seconds on the ui's own clock, and how many frames have been drawn.
 *
 * The screen advances it by the real time between frames, not by the simulation's steps, so a
 * menu still eases open while the game behind it is paused. A test sets it directly.
 **/
struct Clock final {
    float now = 0.0F;
    std::uint64_t frame = 0;
};

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

/**
 * How far something has come since it began to be drawn: 0 on the frame it appears, rising to 1
 * over a duration.
 *
 * Something that was not drawn on the frame before begins again, so a menu eases in each time it
 * opens. Closing needs nothing, because what is closed is not drawn.
 **/
class Opening final {
 public:
    /**
     * @param duration how long opening takes, in seconds. Zero or less is open at once
     * @return how far open, from 0 to 1
     **/
    float progress(const Clock& clock, float duration);

 private:
    float start_ = 0.0F;
    std::uint64_t frame_ = 0;
    bool seen_ = false;
};

/**
 * What a retained component or an immediate widget keeps of its transitions between frames.
 * Layout never reads it.
 **/
struct Motion final {
    Transition face;  /**< the fill behind it **/
    Transition ink;   /**< its text **/
    Transition shortcut;  /**< the shortcut a menu item shows beside its text **/
    Opening opening;  /**< how far it has opened, for what drops or pops up **/
};

};  // namespace v3d::ui
