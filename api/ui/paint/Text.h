/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::paint {

/**
 * The callbacks this library measures and draws text through, so that it names no font
 * type.
 *
 * ComponentRenderer, for the retained components, and Immediate, for the immediate ui, both
 * take this pair. Neither names a font type, so neither needs a device to test, and one
 * TextRenderer supplies both. The pair is declared here, not on either class, so that both
 * classes take the same types.
 *
 * They take a view, not a string, because a component usually already holds the text, and
 * building a string would allocate once per label per frame. The caller keeps the text alive
 * for the length of the call. The callee does not keep the view.
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
 * Text set in another weight or at another size than the body text.
 **/
struct Setting final {
    Measure measure;       /**< how wide a string is in it. Empty sets the text as the body text is set **/
    Write write;
    float scale { 1.0F };  /**< how large it is against the body text **/
};

/**
 * What a centred menu sets apart from the text of its items: its title, and the label of the
 * chosen item. A setting left empty is set as the body text is.
 **/
struct Emphasis final {
    Setting title;
    Setting chosen;
};

/**
 * Break a line at the spaces that do not fit a width.
 *
 * Greedy: a word goes on the current line if it fits, and starts a new one if it does not.
 * A single word wider than the whole width runs past the edge and is not split, so a name
 * or a path stays whole.
 *
 * @param width the room a row has. A width that is not positive puts the whole line on one
 *        row
 * @return one row per line, and no rows at all for a line holding nothing but spaces
 **/
std::vector<std::string> wrap(std::string_view line, float width, const Measure& measure);

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
