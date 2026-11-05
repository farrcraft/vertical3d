/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

namespace v3d::ui::paint {

/**
 * How heavy the face is that text is set in.
 **/
enum class Weight : std::uint8_t {
    Regular,
    SemiBold
};

/**
 * The weight a theme's number names, as CSS numbers weights: below 500 is regular, and 500 and
 * above is semibold.
 **/
Weight weight(float number) noexcept;

/**
 * How far down a widget the baseline of its text sits, as a fraction of the widget's height.
 **/
inline constexpr float WIDGET_BASELINE = 0.7F;

/**
 * How far down a line of text its baseline sits, as a fraction of the line height. The rest
 * is room for descenders.
 **/
inline constexpr float LINE_BASELINE = 0.75F;

};  // namespace v3d::ui::paint
