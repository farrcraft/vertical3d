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
constexpr glm::vec4 PANEL_COLOR{0.05F, 0.06F, 0.09F, 0.92F};
/** an outline **/
constexpr glm::vec4 EDGE_COLOR{0.35F, 0.38F, 0.45F, 1.0F};
/** ordinary text **/
constexpr glm::vec4 TEXT_COLOR{0.78F, 0.80F, 0.84F, 1.0F};
/** text that is disabled, or a placeholder **/
constexpr glm::vec4 DIM_TEXT_COLOR{0.45F, 0.47F, 0.52F, 1.0F};
/** what is drawn behind a selected item **/
constexpr glm::vec4 HIGHLIGHT_COLOR{0.16F, 0.34F, 0.58F, 1.0F};
/** the filled part of a bar **/
constexpr glm::vec4 FILL_COLOR{0.30F, 0.62F, 0.36F, 1.0F};
/** a raised surface: a widget in the immediate ui, a hovered toolbar button in the retained one **/
constexpr glm::vec4 RAISED_COLOR{0.16F, 0.18F, 0.24F, 1.0F};

};  // namespace v3d::ui::paint
