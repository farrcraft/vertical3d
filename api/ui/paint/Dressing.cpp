/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Dressing.h"

#include "Palette.h"

#include <algorithm>
#include <array>
#include <cmath>

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
constexpr float SHADOW_SIZE = 12.0F;
constexpr float SHADOW_OFFSET = 4.0F;
constexpr float OPEN_DISTANCE = 4.0F;
// how long a hover and an opening menu take, in seconds
constexpr float HOVER_DURATION = 0.1F;
constexpr float OPEN_DURATION = 0.12F;

// the sizes the immediate layer starts with where they differ, in pixels
constexpr float TOOLS_LINE_HEIGHT = 18.0F;
constexpr float TOOLS_PADDING = 6.0F;
constexpr float TOOLS_BAR_HEIGHT = 22.0F;
constexpr float TOOLS_SCROLLBAR_WIDTH = 10.0F;
constexpr float TOOLS_RADIUS = 3.0F;

/** every size a scale multiplies, which leaves out the outlines, the durations and the title weight **/
constexpr std::array SIZES{&Dressing::lineHeight, &Dressing::padding, &Dressing::spacing, &Dressing::barHeight,
&Dressing::iconSize, &Dressing::panelPadding, &Dressing::scrollbarWidth, &Dressing::markSize, &Dressing::radius,
&Dressing::indent, &Dressing::shadowSize, &Dressing::shadowOffset, &Dressing::openDistance};

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
shadowSize(SHADOW_SIZE),
shadowOffset(SHADOW_OFFSET),
openDistance(OPEN_DISTANCE),
hoverDuration(HOVER_DURATION),
openDuration(OPEN_DURATION),
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
focus(),
shadow(),
backdrop() {
    Palette().dress(Palette::Use::Chrome, this);
}

Dressing Dressing::scaled(float factor) const noexcept {
    Dressing dressing = *this;
    for (float Dressing::* const size : SIZES) {
        dressing.*size = this->*size * factor;
    }
    dressing.borderWidth = outline(borderWidth, factor);
    dressing.focusWidth = outline(focusWidth, factor);
    return dressing;
}

void Dressing::resize(const Dressing& from) noexcept {
    for (float Dressing::* const size : SIZES) {
        this->*size = from.*size;
    }
    borderWidth = from.borderWidth;
    focusWidth = from.focusWidth;
    titleWeight = from.titleWeight;
    hoverDuration = from.hoverDuration;
    openDuration = from.openDuration;
}

float Dressing::outline(float width, float factor) noexcept {
    if (width <= 0.0F) {
        return 0.0F;
    }
    return std::max(1.0F, std::floor(width * factor));
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
