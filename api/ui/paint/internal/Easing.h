/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Transition.h>

#include <glm/vec4.hpp>

namespace v3d::ui::style {
class Resolver;
};  // namespace v3d::ui::style

namespace v3d::ui::paint::internal {

/**
 * The clock the retained ui is drawn at, and how far each change has eased by it.
 *
 * A colour eases over the base dressing's hover duration, and a menu opens over its open
 * duration. Until a clock is given, every change is drawn at once.
 **/
class Easing final {
 public:
    /**
     * @param styles where the durations come from. Held by reference, because the renderer
     *      owns it, and a theme change must reach the durations
     **/
    explicit Easing(const style::Resolver& styles);

    /**
     * The time the next draw is at.
     **/
    void clock(const Clock& now) noexcept;

    /**
     * @return the time the next draw is at, which is zero until clock() is called
     **/
    const Clock& clock() const noexcept;

    /**
     * @return whether clock() has been called, without which nothing eases
     **/
    bool timed() const noexcept;

    /**
     * Ease a colour toward what is asked for, or fade it out in its own colour when nothing
     * is, over the hover duration.
     **/
    glm::vec4 toward(Transition* transition, const glm::vec4& target) const;
    glm::vec4 away(Transition* transition) const;

    /**
     * How far open something is, from 0 to 1, easing out so it slows as it lands.
     **/
    float opened(Opening* opening) const;

 private:
    const style::Resolver& styles_;
    Clock clock_;
    bool timed_;
};

};  // namespace v3d::ui::paint::internal
