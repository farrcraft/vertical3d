/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <functional>
#include <string_view>

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
 *
 * They take a view rather than a string because what is measured and written is usually
 * already held by a component, and building a string to hand one over is an allocation per
 * label per frame. The view has to outlive the call and nothing keeps it.
 **/

/**
 * How wide a string will be when it is drawn, in pixels.
 **/
typedef std::function<float(std::string_view)> Measure;

/**
 * Draw a string with its pen on the baseline at the given position.
 **/
typedef std::function<void(std::string_view, const glm::vec2&, const glm::vec4&)> Write;

};  // namespace v3d::ui
