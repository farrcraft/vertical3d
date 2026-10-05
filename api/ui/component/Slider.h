/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>
#include <api/ui/Component.h>

#include <glm/vec2.hpp>

namespace v3d::ui::component {

/**
 * A value between a minimum and a maximum, moved in steps, drawn as a track with a thumb.
 *
 * Unlike a scrollbar it holds a value in its own units rather than an offset into content
 * it does not hold, and its thumb does not say how much of anything is shown. It owns the
 * value, the way a list owns which row is chosen, and sends its command each time the value
 * changes - by a press, a drag or a key - so whatever answers reads value() back.
 *
 * The track, the fill up to the thumb and the thumb are the "slider" style class the
 * component names.
 **/
class Slider : public Component {
 public:
    Slider();
    ~Slider() = default;

    /**
     * The values the slider runs between, and the step it moves in. A step of zero or less is
     * continuous. The value is clamped and snapped to the new range.
     **/
    void range(float minimum, float maximum, float step);
    float minimum() const noexcept;
    float maximum() const noexcept;
    float step() const noexcept;

    /**
     * Set the value, clamped to the range and snapped to a whole number of steps from the
     * minimum. The maximum is always reachable, even where the steps do not divide the
     * range.
     *
     * @return whether that changed it
     **/
    bool value(float set);
    float value() const noexcept;

    /**
     * @return where the value stands between the ends, from 0 at the minimum to 1 at the
     *         maximum
     **/
    float fraction() const noexcept;

    /**
     * Set the value under a point, along the track a renderer last left on the slider. A
     * point past either end is that end.
     *
     * @return whether that changed it
     **/
    bool drag(const glm::vec2& point);

    /**
     * Set the event a change sends.
     **/
    void event(const v3d::event::Event& destination);
    v3d::event::Event event() const;

 private:
    float snapped(float set) const noexcept;

    float minimum_;
    float maximum_;
    float step_;
    float value_;
    v3d::event::Event event_;
};

};  // namespace v3d::ui::component
