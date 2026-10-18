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
constexpr float DEFAULT_LINE_HEIGHT = 34.0F;
constexpr float DEFAULT_PADDING = 24.0F;
constexpr float DEFAULT_BAR_HEIGHT = 28.0F;
constexpr float DEFAULT_ICON_SIZE = 22.0F;
constexpr float DEFAULT_PANEL_PADDING = 4.0F;
constexpr float DEFAULT_SCROLLBAR_WIDTH = 12.0F;
constexpr float DEFAULT_MARK_SIZE = 16.0F;

// the colours a dressing starts with that the immediate ui does not share

/** the unfilled part of a bar, and a tab that is not chosen **/
constexpr glm::vec4 TRACK_COLOR{0.12F, 0.13F, 0.17F, 1.0F};
/** a mark, the caret and the focus ring **/
constexpr glm::vec4 ACCENT_COLOR{0.42F, 0.66F, 0.95F, 1.0F};

};  // namespace

Dressing::Dressing() noexcept :
lineHeight(DEFAULT_LINE_HEIGHT),
padding(DEFAULT_PADDING),
barHeight(DEFAULT_BAR_HEIGHT),
iconSize(DEFAULT_ICON_SIZE),
panelPadding(DEFAULT_PANEL_PADDING),
scrollbarWidth(DEFAULT_SCROLLBAR_WIDTH),
markSize(DEFAULT_MARK_SIZE),
borderWidth(1.0F),
focusWidth(2.0F),
radius(0.0F),
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
activeText(1.0F, 1.0F, 1.0F, 1.0F),
disabledText(DIM_TEXT_COLOR),
highlight(HIGHLIGHT_COLOR),
hover(RAISED_COLOR),
focus(ACCENT_COLOR) {
}

};  // namespace v3d::ui::paint
