/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <optional>

#include <boost/json/object.hpp>

#include <glm/vec2.hpp>

namespace v3d::config {

/**
 * The size a window document gives the window, from {"window": {"width": w, "height": h}}.
 *
 * Each dimension is a whole number an int can hold. One beyond that range is refused, not
 * narrowed to a different number.
 *
 * @return the width and height, or nothing when the document has no window object or either
 *         dimension is not such a number
 **/
std::optional<glm::ivec2> windowSize(const boost::json::object& document);

};  // namespace v3d::config
