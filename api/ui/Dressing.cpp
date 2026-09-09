/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Dressing.h"

namespace v3d::ui {

Dressing::Dressing() noexcept :
lineHeight(34.0f),
padding(24.0f),
barHeight(28.0f),
iconSize(22.0f),
panelPadding(4.0f),
scrollbarWidth(12.0f),
markSize(16.0f),
borderWidth(1.0f),
focusWidth(2.0f),
radius(0.0f),
panel(0.05f, 0.06f, 0.09f, 0.92f),
border(0.35f, 0.38f, 0.45f, 1.0f),
track(0.12f, 0.13f, 0.17f, 1.0f),
fill(0.30f, 0.62f, 0.36f, 1.0f),
thumb(0.35f, 0.38f, 0.45f, 1.0f),
mark(0.42f, 0.66f, 0.95f, 1.0f),
text(0.78f, 0.80f, 0.84f, 1.0f),
activeText(1.0f, 1.0f, 1.0f, 1.0f),
highlight(0.16f, 0.34f, 0.58f, 1.0f),
hover(0.16f, 0.18f, 0.24f, 1.0f),
focus(0.42f, 0.66f, 0.95f, 1.0f) {
}

};  // namespace v3d::ui
