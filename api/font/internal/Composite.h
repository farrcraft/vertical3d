/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/ColourGlyph.h>

#include <glm/vec4.hpp>

namespace v3d::font::internal {

/**
 * Put one colour on another by a composite mode, as the W3C's compositing and blending rules
 * define each. Both colours and the result are premultiplied: red, green and blue already times
 * alpha.
 *
 * @param source the colour put down
 * @param destination the colour beneath it
 **/
glm::vec4 composite(const glm::vec4& source, const glm::vec4& destination, ColourComposite mode);

};  // namespace v3d::font::internal
