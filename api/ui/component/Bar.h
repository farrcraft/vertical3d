/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Component.h>

namespace v3d::ui::component {

/**
 * A fill as a fraction of a track - a hit point bar, a cooldown, a progress readout.
 *
 * The track is the component's whole box and the fill is the part of it the fraction
 * covers, so a bar is sized by its layout like anything else. A horizontal bar fills from
 * the left and a vertical one from the bottom, which is the direction each is read in.
 *
 * The track and the fill are drawn in the "bar" style class the component names, per
 * ADR-0020.
 **/
class Bar : public Component {
 public:
    enum class Direction {
        Horizontal,
        Vertical
    };

    Bar();
    ~Bar() = default;

    /**
     * How much of the track is filled, clamped to 0..1.
     **/
    void fraction(float fill);
    float fraction() const noexcept;

    void direction(Direction fill);
    Direction direction() const noexcept;

 private:
    float fraction_;
    Direction direction_;
};

};  // namespace v3d::ui::component
