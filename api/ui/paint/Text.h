/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::paint {

/**
 * A face's weight, and the callbacks a string's width is found and the string drawn through
 * where no text renderer is held: the cursor placing a text box's caret, and the editor's
 * overlays.
 *
 * They take a view, not a string, so that measuring a label does not allocate. The caller keeps
 * the text alive for the length of the call. The callee does not keep the view.
 **/

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
 * How wide a string will be when it is drawn, in pixels.
 **/
typedef std::function<float(std::string_view)> Measure;

/**
 * Draw a string with its pen on the baseline at the given position.
 **/
typedef std::function<void(std::string_view, const glm::vec2&, const glm::vec4&)> Write;

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
