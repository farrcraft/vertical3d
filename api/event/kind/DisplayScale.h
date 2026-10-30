/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::event::kind {

/**
 * The scale of the monitor the window is on changed, because the user changed it or the window
 * moved to a monitor with another.
 *
 * The scale is how much larger than its pixel size a ui is drawn to look the size it was
 * designed at: 1 at 100%, 1.5 at 150%.
 **/
class DisplayScale final {
 public:
    /**
     **/
    explicit DisplayScale(float scale) noexcept;
    float scale() const noexcept;

 private:
     float scale_;
};
};  // namespace v3d::event::kind
