/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec4.hpp>

namespace v3d::ui::paint {

/**
 * The default colours that the retained and the immediate ui share, so that the two draw
 * with one look until a theme says otherwise.
 **/

/** the background of a panel or a window, slightly see-through **/
constexpr glm::vec4 PANEL_COLOR{0.05f, 0.06f, 0.09f, 0.92f};
/** an outline **/
constexpr glm::vec4 EDGE_COLOR{0.35f, 0.38f, 0.45f, 1.0f};
/** ordinary text **/
constexpr glm::vec4 TEXT_COLOR{0.78f, 0.80f, 0.84f, 1.0f};
/** text that is disabled, or a placeholder **/
constexpr glm::vec4 DIM_TEXT_COLOR{0.45f, 0.47f, 0.52f, 1.0f};
/** what is drawn behind a selected item **/
constexpr glm::vec4 HIGHLIGHT_COLOR{0.16f, 0.34f, 0.58f, 1.0f};
/** the filled part of a bar **/
constexpr glm::vec4 FILL_COLOR{0.30f, 0.62f, 0.36f, 1.0f};
/** a raised surface: a widget in the immediate ui, a hovered toolbar button in the retained one **/
constexpr glm::vec4 RAISED_COLOR{0.16f, 0.18f, 0.24f, 1.0f};

};  // namespace v3d::ui::paint
