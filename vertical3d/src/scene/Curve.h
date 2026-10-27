/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/curve/Curves.h>

#include <cstdint>

#include "Item.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * Curves placed in the scene.
 *
 * The geometry is a curve::Curves described about the item's own origin, which may hold any
 * number of curves, each of its own type. The item's transform places it. The geometry is
 * shared rather than owned, so two items may show the same curves.
 *
 * Most curves are guides that shape other things and are not seen in a render, so a curve
 * renders only when its style says so.
 **/
class Curve final : public Item {
 public:
    static constexpr Kind KIND = Kind::Curve;

    /**
     * A fiftieth of a unit, thin enough to read as a line and wide enough to be seen.
     **/
    static constexpr float defaultWidth = 0.02F;

    /**
     * How the curves are rendered.
     **/
    struct Style {
        /**
         * Whether a render draws the curves, as ribbons of the width.
         **/
        bool renders = false;
        /**
         * The ribbon's width, in the item's own units.
         **/
        float width = defaultWidth;

        /**
         * Whether the width is finite and above zero. A scene document holds only a valid
         * style.
         **/
        bool valid() const;

        bool operator==(const Style& other) const = default;
    };

    /**
     * @param geometry what the item shows. An empty pointer is replaced by geometry with no
     *        curves, so geometry() is never empty
     **/
    explicit Curve(const boost::shared_ptr<v3d::curve::Curves>& geometry);

    Kind kind() const noexcept override;

    const boost::shared_ptr<v3d::curve::Curves>& geometry() const noexcept;

    const Style& style() const noexcept;
    void style(const Style& style) noexcept;

    /**
     * The item's revision, which counts the geometry's changes as well
     **/
    std::uint64_t revision() const noexcept override;

 private:
    boost::shared_ptr<v3d::curve::Curves> geometry_;
    Style style_;
};

};  // namespace v3d::editor
