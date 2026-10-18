/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Dressing.h"

#include <api/ui/paint/Palette.h>

namespace v3d::ui::paint {

namespace {

/**
 * The sizes a dressing starts with, in pixels.
 **/
constexpr float DEFAULT_LINE_HEIGHT = 34.0f;
constexpr float DEFAULT_PADDING = 24.0f;
constexpr float DEFAULT_BAR_HEIGHT = 28.0f;
constexpr float DEFAULT_ICON_SIZE = 22.0f;
constexpr float DEFAULT_PANEL_PADDING = 4.0f;
constexpr float DEFAULT_SCROLLBAR_WIDTH = 12.0f;
constexpr float DEFAULT_MARK_SIZE = 16.0f;

// the colours a dressing starts with that the immediate ui does not share

/** the unfilled part of a bar, and a tab that is not chosen **/
constexpr glm::vec4 TRACK_COLOR{0.12f, 0.13f, 0.17f, 1.0f};
/** a mark, the caret and the focus ring **/
constexpr glm::vec4 ACCENT_COLOR{0.42f, 0.66f, 0.95f, 1.0f};

};  // namespace

Dressing::Dressing() noexcept :
lineHeight(DEFAULT_LINE_HEIGHT),
padding(DEFAULT_PADDING),
barHeight(DEFAULT_BAR_HEIGHT),
iconSize(DEFAULT_ICON_SIZE),
panelPadding(DEFAULT_PANEL_PADDING),
scrollbarWidth(DEFAULT_SCROLLBAR_WIDTH),
markSize(DEFAULT_MARK_SIZE),
borderWidth(1.0f),
focusWidth(2.0f),
radius(0.0f),
panel(PANEL_COLOR),
border(EDGE_COLOR),
track(TRACK_COLOR),
fill(FILL_COLOR),
thumb(EDGE_COLOR),
mark(ACCENT_COLOR),
caret(ACCENT_COLOR),
placeholder(DIM_TEXT_COLOR),
tab(TRACK_COLOR),
text(TEXT_COLOR),
activeText(1.0f, 1.0f, 1.0f, 1.0f),
disabledText(DIM_TEXT_COLOR),
highlight(HIGHLIGHT_COLOR),
hover(RAISED_COLOR),
focus(ACCENT_COLOR) {
}

};  // namespace v3d::ui::paint
