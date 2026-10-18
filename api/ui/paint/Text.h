/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

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
 * How wide a string will be when it is drawn, in pixels.
 **/
typedef std::function<float(std::string_view)> Measure;

/**
 * Draw a string with its pen on the baseline at the given position.
 **/
typedef std::function<void(std::string_view, const glm::vec2&, const glm::vec4&)> Write;

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

};  // namespace v3d::ui::paint
