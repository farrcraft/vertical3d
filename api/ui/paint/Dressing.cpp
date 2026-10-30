/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Dressing.h"

#include "Palette.h"

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
/** semibold, as CSS numbers it **/
constexpr float TITLE_WEIGHT = 600.0F;

// the sizes the immediate layer starts with where they differ, in pixels
constexpr float TOOLS_LINE_HEIGHT = 18.0F;
constexpr float TOOLS_PADDING = 6.0F;
constexpr float TOOLS_BAR_HEIGHT = 22.0F;
constexpr float TOOLS_SCROLLBAR_WIDTH = 10.0F;
constexpr float TOOLS_RADIUS = 3.0F;

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
titleWeight(TITLE_WEIGHT),
gap(),
bar(),
panel(),
border(),
titleBar(),
track(),
fill(),
thumb(),
mark(),
caret(),
placeholder(),
tab(),
widget(),
rule(),
hairline(),
text(),
activeText(),
secondaryText(),
disabledText(),
highlight(),
selected(),
hover(),
pressed(),
focus() {
    Palette().dress(Palette::Use::Chrome, this);
}

Dressing Dressing::tools() noexcept {
    Dressing dressing;
    dressing.lineHeight = TOOLS_LINE_HEIGHT;
    dressing.padding = TOOLS_PADDING;
    dressing.barHeight = TOOLS_BAR_HEIGHT;
    dressing.scrollbarWidth = TOOLS_SCROLLBAR_WIDTH;
    dressing.radius = TOOLS_RADIUS;
    Palette().dress(Palette::Use::Tools, &dressing);
    return dressing;
}

};  // namespace v3d::ui::paint
