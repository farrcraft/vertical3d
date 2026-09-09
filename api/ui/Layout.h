/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/Bound2D.h>

#include <glm/vec2.hpp>

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

/**
 * Where a component sits in the box around it, and how big it is there.
 *
 * This is the input to layout; Component::position() and size() are the output, which is
 * the absolute box it was last drawn in per ADR-0019. The two are separate so that a
 * percentage survives being resolved: a component that stored only the resolved number
 * would forget what it asked for the moment its parent changed size.
 *
 * Nothing here reads the output. A box is worked out from the box around it and from what
 * the component makes of itself, so the answer is the same however many times it is asked
 * and whatever was on the screen before - ADR-0039.
 *
 * x and y are measured from the anchored corner and grow inwards, so a bottom right
 * anchor with an x of 8 sits eight pixels in from the right edge whatever the parent's
 * width is.
 **/
struct Layout final {
    enum class Anchor {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
        Centre
    };

    Layout() noexcept;

    /**
     * Work out the absolute box this describes inside a parent.
     *
     * @param parent the box the component sits in, which is the canvas for a component
     *      with no parent
     * @param own the size the component makes of itself, for an Auto extent
     **/
    v3d::type::geometry::Bound2D resolve(const v3d::type::geometry::Bound2D& parent, const glm::vec2& own) const;

    Length x;
    Length y;
    Length width;
    Length height;
    Anchor anchor;
};

};  // namespace v3d::ui
