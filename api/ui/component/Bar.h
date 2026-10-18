/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Component.h>

#include <cstdint>

namespace v3d::ui::component {

/**
 * A fill as a fraction of a track - a hit point bar, a cooldown, a progress readout.
 *
 * The track is the component's whole box, and the fill is the part of it the fraction
 * covers, which means a bar is sized by its layout like any other component. A horizontal
 * bar fills from the left, and a vertical one from the bottom.
 *
 * The track and the fill are drawn in the "bar" style class the component names.
 **/
class Bar : public Component {
 public:
    enum class Direction : std::uint8_t {
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
