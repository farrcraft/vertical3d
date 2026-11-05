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
submenuDelay(SUBMENU_DELAY),
tooltipDelay(TOOLTIP_DELAY),
tooltipQuickDelay(TOOLTIP_QUICK_DELAY),
plateOpacity(PLATE_OPACITY),
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
chosen(),
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
    submenuDelay = from.submenuDelay;
    tooltipDelay = from.tooltipDelay;
    tooltipQuickDelay = from.tooltipQuickDelay;
    plateOpacity = from.plateOpacity;
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
