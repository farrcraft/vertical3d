/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Dressing.h"

namespace v3d::ui::paint {

namespace {

// the sizes the retained ui starts with, in pixels
constexpr float LINE_HEIGHT = 34.0F;
constexpr float PADDING = 24.0F;
constexpr float SPACING = 4.0F;
constexpr float BAR_HEIGHT = 28.0F;
constexpr float ICON_SIZE = 22.0F;
constexpr float PANEL_PADDING = 4.0F;
constexpr float SCROLLBAR_WIDTH = 12.0F;
constexpr float MARK_SIZE = 16.0F;
constexpr float FOCUS_WIDTH = 2.0F;
constexpr float INDENT = 14.0F;

// the sizes the immediate layer starts with where they differ, in pixels
constexpr float TOOLS_LINE_HEIGHT = 18.0F;
constexpr float TOOLS_PADDING = 6.0F;
constexpr float TOOLS_BAR_HEIGHT = 22.0F;
constexpr float TOOLS_SCROLLBAR_WIDTH = 10.0F;
constexpr float TOOLS_RADIUS = 3.0F;

// the colours both ui models start from, so that the two look the same

/** the background of a panel or a window, slightly see-through **/
constexpr glm::vec4 PANEL_COLOR{0.05F, 0.06F, 0.09F, 0.92F};
/** an outline, and a scrollbar's thumb **/
constexpr glm::vec4 EDGE_COLOR{0.35F, 0.38F, 0.45F, 1.0F};
/** ordinary text **/
constexpr glm::vec4 TEXT_COLOR{0.78F, 0.80F, 0.84F, 1.0F};
/** the text of the chosen or hovered item **/
constexpr glm::vec4 BRIGHT_TEXT_COLOR{1.0F, 1.0F, 1.0F, 1.0F};
/** text that is disabled or secondary, or a placeholder **/
constexpr glm::vec4 DIM_TEXT_COLOR{0.45F, 0.47F, 0.52F, 1.0F};
/** what is drawn behind a selected item **/
constexpr glm::vec4 HIGHLIGHT_COLOR{0.16F, 0.34F, 0.58F, 1.0F};
/** the filled part of a bar **/
constexpr glm::vec4 FILL_COLOR{0.30F, 0.62F, 0.36F, 1.0F};
/** a raised surface: an immediate widget at rest, a hovered toolbar button **/
constexpr glm::vec4 RAISED_COLOR{0.16F, 0.18F, 0.24F, 1.0F};
/** a raised surface the cursor is on, which has to read against RAISED_COLOR **/
constexpr glm::vec4 LIFTED_COLOR{0.22F, 0.25F, 0.33F, 1.0F};
/** the unfilled part of a bar, and a tab that is not chosen **/
constexpr glm::vec4 TRACK_COLOR{0.12F, 0.13F, 0.17F, 1.0F};
/** a window's title bar and a table's header **/
constexpr glm::vec4 TITLE_BAR_COLOR{0.12F, 0.14F, 0.19F, 1.0F};
/** a separator **/
constexpr glm::vec4 RULE_COLOR{0.24F, 0.26F, 0.32F, 1.0F};
/** a mark, the caret and the focus ring **/
constexpr glm::vec4 ACCENT_COLOR{0.42F, 0.66F, 0.95F, 1.0F};

};  // namespace

Dressing::Dressing() noexcept :
lineHeight(LINE_HEIGHT),
padding(PADDING),
spacing(SPACING),
barHeight(BAR_HEIGHT),
iconSize(ICON_SIZE),
panelPadding(PANEL_PADDING),
scrollbarWidth(SCROLLBAR_WIDTH),
markSize(MARK_SIZE),
borderWidth(1.0F),
focusWidth(FOCUS_WIDTH),
radius(0.0F),
indent(INDENT),
panel(PANEL_COLOR),
border(EDGE_COLOR),
titleBar(TITLE_BAR_COLOR),
track(TRACK_COLOR),
fill(FILL_COLOR),
thumb(EDGE_COLOR),
mark(ACCENT_COLOR),
caret(ACCENT_COLOR),
placeholder(DIM_TEXT_COLOR),
tab(TRACK_COLOR),
widget(RAISED_COLOR),
rule(RULE_COLOR),
text(TEXT_COLOR),
activeText(BRIGHT_TEXT_COLOR),
disabledText(DIM_TEXT_COLOR),
highlight(HIGHLIGHT_COLOR),
hover(RAISED_COLOR),
focus(ACCENT_COLOR) {
}

Dressing Dressing::tools() noexcept {
    Dressing dressing;
    dressing.lineHeight = TOOLS_LINE_HEIGHT;
    dressing.padding = TOOLS_PADDING;
    dressing.barHeight = TOOLS_BAR_HEIGHT;
    dressing.scrollbarWidth = TOOLS_SCROLLBAR_WIDTH;
    dressing.radius = TOOLS_RADIUS;
    // an immediate widget at rest is already raised, so the cursor lifts it further, and a
    // track or a tab sits at the widget's level
    dressing.track = RAISED_COLOR;
    dressing.tab = RAISED_COLOR;
    dressing.hover = LIFTED_COLOR;
    return dressing;
}

};  // namespace v3d::ui::paint
