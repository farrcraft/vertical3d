/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::ui {

/**
 * How far, or how big, in one axis.
 *
 * Percent is of the parent's extent in the same axis, so a width of 50% is half as wide as
 * the box around it. Auto leaves the number to whatever asked. For a size, that is the size
 * the component needs on the axis: the width of a label's text, or the side of an icon. A
 * component that needs nothing takes the room it was offered. For a position, Auto is the
 * anchored corner itself.
 **/
class Length final {
 public:
    enum class Unit {
        Auto,
        Pixels,
        Percent
    };

    /**
     * An Auto length, which a component that names nothing is laid out with.
     **/
    Length() noexcept;

    Length(float value, Unit unit) noexcept;

    float value() const noexcept;
    Unit unit() const noexcept;

    /**
     * @param extent the parent's extent in this axis, which a Percent is of
     * @param own what the component makes of this axis itself, which Auto resolves to
     * @return the length in pixels
     **/
    float resolve(float extent, float own) const noexcept;

 private:
    float value_;
    Unit unit_;
};

};  // namespace v3d::ui
