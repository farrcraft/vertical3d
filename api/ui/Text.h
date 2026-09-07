/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <functional>
#include <string>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui {

/**
 * The seam between this library and whatever draws its text, per ADR-0019.
 *
 * Both ways of writing a ui take this pair: ComponentRenderer for the retained components
 * and Immediate for the layer of calls. Neither names a font type, so neither costs a
 * device to test, and one TextRenderer supplies both.
 *
 * They are declared here rather than on either class so that they are one type rather than
 * two that happen to have the same signature - a pair built for one side would otherwise
 * only work on the other by coincidence.
 **/

/**
 * How wide a string will be when it is drawn, in pixels.
 **/
typedef std::function<float(const std::string&)> Measure;

/**
 * Draw a string with its pen on the baseline at the given position.
 **/
typedef std::function<void(const std::string&, const glm::vec2&, const glm::vec4&)> Write;

};  // namespace v3d::ui
