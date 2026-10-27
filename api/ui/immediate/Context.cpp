/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Context.h"

#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Painter.h>
#include <api/ui/paint/Text.h>

#include <algorithm>
#include <cstdint>
#include <string>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::immediate {

namespace {

/**
 * The seed and the multiplier of FNV-1a, which hashes a label into an id.
 * Any hash would do; this one is four lines and needs no table.
 **/
const std::uint64_t hashSeed = 1469598103934665603ULL;
const std::uint64_t hashPrime = 1099511628211ULL;

};  // namespace

Reaction::Reaction() noexcept :
hovered(false),
held(false),
clicked(false) {
}

Retained::Retained() noexcept :
frame(0),
tab(0),
scroll(0.0F),
content(0.0F),
offset(0.0F, 0.0F),
collapsed(false),
dragging(false),
chosen(-1) {
}

Popup::Popup() noexcept :
id(0),
asked(false),
boxMin(0.0F, 0.0F),
boxMax(0.0F, 0.0F),
selected(-1) {
}

Row::Row() noexcept :
margin(0.0F),
right(0.0F),
penY(0.0F),
top(0.0F),
height(0.0F),
lastRight(0.0F),
sameLine(false) {
}

Window::Window() noexcept :
open(false),
margin(0.0F),
right(0.0F),
id(0),
scroll(0),
bodyMin(0.0F, 0.0F),
bodyMax(0.0F, 0.0F),
contentTop(0.0F),
scrolls(false),
clipped(false) {
}

TabStrip::TabStrip() noexcept :
id(0),
open(false),
pen(0.0F),
top(0.0F),
index(0),
wanted(0),
changed(false),
taken(false) {
}

Table::Table() noexcept :
open(false),
left(0.0F),
column(0),
id(0),
scroll(0),
right(0.0F),
height(0.0F),
top(0.0F),
contentTop(0.0F),
clipped(false) {
}

Context::Context(const paint::Measure& measurer, const paint::Write& writer) :
    measure(measurer),
    write(writer),
    dressing(paint::Dressing::tools()),
    canvas(nullptr),
    previousCursor(0.0F, 0.0F),
    drag(0.0F, 0.0F),
    hovered(0),
    hovering(0),
    active(0),
    pressAt(0.0F, 0.0F),
    nextWidth(0.0F),
    frame(0),
    disabled(0),
    wheeled(0),
    typing(0),
    letGo(false),
    typingDrawn(false),
    itemMin(0.0F, 0.0F),
    itemMax(0.0F, 0.0F),
    itemOwner(0) {
}

void Context::focus(Id field) {
    const bool was = typing != 0;
    typing = field;
    letGo = false;
    if (field == 0) {
        written.clear();
    }
    const bool now = typing != 0;
    if (was != now && onTyping) {
        onTyping(now);
    }
}

Retained& Context::retain(Id id) {
    Retained& retained = states[id];
    retained.frame = frame;
    return retained;
}

Id Context::identify(const std::string& label) const {
    std::uint64_t value = ids.empty() ? hashSeed : ids.back();
    for (const char letter : label) {
        value ^= static_cast<std::uint64_t>(static_cast<unsigned char>(letter));
        value *= hashPrime;
    }
    // an id of zero means "nothing", so a label that hashes to zero is given 1 instead
    return value == 0 ? 1 : value;
}

float Context::itemWidth() {
    // measured from where the widget will be placed. On a shared row that is past the last
    // widget, and otherwise at the margin. place() has not run yet, so this works out the
    // same corner it will
    const float left = row.sameLine ? row.lastRight + dressing.spacing : row.margin;
    const float rest = std::max(row.right - left, 0.0F);
    const float width = nextWidth > 0.0F ? std::min(nextWidth, rest) : rest;
    nextWidth = 0.0F;
    return width;
}

glm::vec2 Context::place(const glm::vec2& size) {
    glm::vec2 corner;
    if (row.sameLine) {
        corner = glm::vec2(row.lastRight + dressing.spacing, row.top);
        row.sameLine = false;
    } else {
        row.top = row.penY;
        row.height = 0.0F;
        corner = glm::vec2(row.margin, row.top);
    }
    row.lastRight = corner.x + size.x;
    row.height = std::max(row.height, size.y);
    row.penY = row.top + row.height + dressing.spacing;

    // an item that offers its box to the cursor claims it in interact(). One that does not,
    // such as text, is hovered when the window it is in holds the cursor, or when nothing
    // does outside a window
    itemMin = corner;
    itemMax = corner + size;
    itemOwner = window.open ? window.id : 0;
    return corner;
}

Reaction Context::interact(Id id, const glm::vec2& min, const glm::vec2& max) {
    Reaction reaction;
    if (disabled > 0) {
        return reaction;
    }
    if (min == itemMin && max == itemMax) {
        // the widget just placed is offering its own box, so it is what has to hold the
        // cursor for itemHovered(). A title bar or a scrollbar offers a box place() never
        // made, and leaves the last item alone
        itemOwner = id;
    }
    const bool within = paint::inside(min, max, input.cursor);
    if (within) {
        hovering = id;
    }
    reaction.hovered = within && hovered == id;
    if (reaction.hovered && input.pressed) {
        active = id;
        pressAt = input.cursor;
    }
    reaction.held = active == id;
    reaction.clicked = reaction.held && reaction.hovered && input.released;
    return reaction;
}

glm::vec4 Context::ink(const glm::vec4& colour) const {
    return disabled > 0 ? dressing.disabledText : colour;
}

glm::vec4 Context::face(bool lit, bool under) const {
    if (disabled > 0) {
        return dressing.widget;
    }
    if (lit) {
        return dressing.highlight;
    }
    return under ? dressing.hover : dressing.widget;
}

void Context::label(const std::string& line, const glm::vec2& min, const glm::vec2& size,
    const glm::vec4& colour) const {
    // the baseline sits a quarter of a line below the box's middle, which is three quarters
    // of the way down a box one line tall. A font with ordinary descenders looks centred there
    const glm::vec2 pen(min.x, min.y + ((size.y + (dressing.lineHeight * 0.5F)) * 0.5F));
    write(line, pen, colour);
}

};  // namespace v3d::ui::immediate
