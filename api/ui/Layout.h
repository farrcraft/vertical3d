/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/Bound2D.h>

#include <cstdint>

#include "Length.h"

#include <glm/vec2.hpp>

namespace v3d::ui {

/**
 * Where a component sits in the box around it, and how big it is there.
 *
 * This is the input to layout. Component::position() and size() are the output, the
 * absolute box the component was last drawn in. The two are kept apart so that a
 * percentage is kept after it is resolved, and is resolved again when the parent changes
 * size.
 *
 * Nothing here reads the output. A box is worked out from the box around it and from the
 * component's natural size. The result is the same however many times it is resolved, and
 * whatever was on the screen before.
 *
 * x and y are measured from the anchored corner and grow inwards, so a bottom right
 * anchor with an x of 8 sits eight pixels in from the right edge whatever the parent's
 * width is.
 **/
struct Layout final {
    enum class Anchor : std::uint8_t {
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
     * @param own the component's natural size, which an Auto extent resolves to
     **/
    v3d::type::geometry::Bound2D resolve(const v3d::type::geometry::Bound2D& parent, const glm::vec2& own) const;

    Length x;
    Length y;
    Length width;
    Length height;
    Anchor anchor;
};

};  // namespace v3d::ui
