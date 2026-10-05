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
 * the box around it. Auto hands the number back to whatever asked: for a size that is what
 * the component makes of the axis - the width of a label's text, the side of an icon, the
 * room it was offered when it makes nothing - and for a position it is the anchored corner
 * itself. ADR-0039.
 **/
class Length final {
 public:
    enum class Unit {
        Auto,
        Pixels,
        Percent
    };

    /**
     * An Auto length, which is what a component that names nothing is laid out with.
     **/
    Length() noexcept;

    Length(float value, Unit unit) noexcept;

    float value() const noexcept;
    Unit unit() const noexcept;

    /**
     * @param extent the parent's extent in this axis, which a Percent is of
     * @param own what the component makes of this axis itself, which is what Auto is
     * @return the length in pixels
     **/
    float resolve(float extent, float own) const noexcept;

 private:
    float value_;
    Unit unit_;
};

};  // namespace v3d::ui
