/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

namespace v3d::ui {

/**
 * How far, or how big, in one axis.
 *
 * Percent is of the parent's extent in the same axis, so a width of 50% is half as wide as
 * the box around it. Auto resolves to the value the caller passes in Basis::own. For a size,
 * that is the size the component needs on the axis: the width of a label's text, or the
 * side of an icon. A component that needs nothing takes the room it was offered. For a
 * position, Auto is no offset from the anchored corner.
 **/
class Length final {
 public:
    enum class Unit : std::uint8_t {
        Auto,
        Pixels,
        Percent
    };

    /**
     * An Auto length. A component that sets no length is laid out with this.
     **/
    Length() noexcept;

    Length(float value, Unit unit) noexcept;

    float value() const noexcept;
    Unit unit() const noexcept;

    /**
     * What a length is resolved against in one axis. A Percent is of extent, the parent's
     * size. Auto resolves to own, the size the component needs.
     **/
    struct Basis {
        float extent = 0.0F;
        float own = 0.0F;
    };

    /**
     * @return the length in pixels
     **/
    float resolve(const Basis& basis) const noexcept;

 private:
    float value_;
    Unit unit_;
};

};  // namespace v3d::ui
