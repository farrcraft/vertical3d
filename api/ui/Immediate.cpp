/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Immediate.h"

#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/geometry/Bound2D.h>
#include <api/ui/component/Scrollbar.h>
#include <api/ui/input/LineEdit.h>
#include <api/ui/paint/Painter.h>
#include <api/ui/paint/Text.h>
#include <api/ui/style/Resolver.h>
#include <api/ui/style/Style.h>
#include <api/ui/style/Theme.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace v3d::ui {

namespace {

/**
 * The seed and the multiplier of FNV-1a, which hashes a label into an id.
 * Any hash would do; this one is four lines and needs no table.
 **/
const std::uint64_t hashSeed = 1469598103934665603ULL;
const std::uint64_t hashPrime = 1099511628211ULL;

/**
 * How thick a separator and the line under a tab strip are.
 **/
const float ruleWidth = 1.0F;

/**
 * How far a scrub moves the value for each pixel the cursor travels. One unit per pixel is
 * too fast to land on a number by hand and too slow to cross a wide range; a quarter is a
 * compromise between the two.
 **/
const float scrubRate = 0.25F;

/**
 * How far one notch of the wheel scrolls a window, as a multiple of a row. Three rows is
 * what a desktop scrolls by, and reads as a deliberate move rather than a nudge.
 **/
const float wheelRows = 3.0F;

/**
 * How far a press on a title bar has to travel before it moves the window instead of
 * folding it. Wide enough that a hand resting on the button does not drag,
 * and narrow enough that a move that was meant is never read as a fold.
 **/
const float dragThreshold = 3.0F;

/**
 * The side of the square a bullet is drawn in, as a fraction of a row.
 **/
const float bulletScale = 0.28F;

/**
 * How many segments a bullet is drawn with.
 **/
const unsigned int bulletSides = 8;

/**
 * How far into its box a checkbox's tick is inset, as a fraction of the box's side.
 **/
const float tickInset = 0.25F;

/**
 * How wide the mark at the right end of a combo is, as a fraction of a row.
 **/
const float comboMark = 0.4F;

};  // namespace

Immediate::Input::Input() noexcept :
cursor(0.0F, 0.0F),
down(false),
pressed(false),
released(false),
wheel(0.0F) {
}

Immediate::Reaction::Reaction() noexcept :
hovered(false),
held(false),
clicked(false) {
}

Immediate::Retained::Retained() noexcept :
frame(0),
tab(0),
scroll(0.0F),
content(0.0F),
offset(0.0F, 0.0F),
collapsed(false),
dragging(false),
chosen(-1) {
}

Immediate::Popup::Popup() noexcept :
id(0),
asked(false),
boxMin(0.0F, 0.0F),
boxMax(0.0F, 0.0F),
selected(-1) {
}

Immediate::Row::Row() noexcept :
margin(0.0F),
right(0.0F),
penY(0.0F),
top(0.0F),
height(0.0F),
lastRight(0.0F),
sameLine(false) {
}

Immediate::Window::Window() noexcept :
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

Immediate::TabStrip::TabStrip() noexcept :
id(0),
open(false),
pen(0.0F),
top(0.0F),
index(0),
wanted(0),
changed(false),
taken(false) {
}

Immediate::Table::Table() noexcept :
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

Immediate::Immediate(const paint::Measure& measure, const paint::Write& write) :
    measure_(measure),
    write_(write),
    dressing_(paint::Dressing::tools()),
    canvas_(nullptr),
    previousCursor_(0.0F, 0.0F),
    drag_(0.0F, 0.0F),
    hovered_(0),
    hovering_(0),
    active_(0),
    pressAt_(0.0F, 0.0F),
    nextWidth_(0.0F),
    frame_(0),
    disabled_(0),
    wheeled_(0),
    typing_(0),
    letGo_(false),
    typingDrawn_(false),
    itemMin_(0.0F, 0.0F),
    itemMax_(0.0F, 0.0F),
    itemOwner_(0) {
}

// out of line, so that the header need not complete the types the members hold
Immediate::~Immediate() {
}

/**
 **/
void Immediate::text(const paint::Measure& measure, const paint::Write& write) {
    measure_ = measure;
    write_ = write;
}

paint::Dressing& Immediate::dressing() noexcept {
    return dressing_;
}

void Immediate::theme(const boost::shared_ptr<style::Theme>& theme) {
    // the new theme starts from what was there before any theme, so where it is silent the
    // last theme's value does not linger
    overlay_.restore(&dressing_);
    style::readBase(style::lookup(theme, style::Resolver::tools, std::string_view()), &overlay_, &dressing_);
}

void Immediate::begin(v3d::render::realtime::canvas::Canvas* canvas, const Input& input) {
    canvas_ = canvas;
    input_ = input;
    frame_++;
    drag_ = input.cursor - previousCursor_;
    hovering_ = 0;
    nextWidth_ = 0.0F;
    ids_.clear();
    disabled_ = 0;
    window_.open = false;
    window_.clipped = false;
    wheeled_ = 0;
    tabs_.open = false;
    table_.open = false;
    table_.clipped = false;
    row_.sameLine = false;
    row_.margin = 0.0F;
    row_.penY = 0.0F;
    row_.right = canvas == nullptr ? 0.0F : static_cast<float>(canvas->width());
    typingDrawn_ = false;
    itemMin_ = glm::vec2(0.0F, 0.0F);
    itemMax_ = glm::vec2(0.0F, 0.0F);
    itemOwner_ = 0;
    popup_.asked = false;
}

void Immediate::end() {
    // the open combo's list goes over everything, so it is drawn after everything, and takes
    // the cursor from whatever it covers
    popup();

    // a field that was not drawn cannot be typed into, so it does not keep the keys from the
    // app's bindings
    if (typing_ != 0 && !typingDrawn_) {
        focus(0);
    }

    // what the cursor was found on this frame is used by the next one, so a window drawn
    // later takes the cursor from one under it
    hovered_ = hovering_;
    if (input_.released || !input_.down) {
        active_ = 0;
    }
    previousCursor_ = input_.cursor;
    canvas_ = nullptr;

    // what nothing has asked for in a while goes, so that a caller building ids out of
    // changing text costs a bounded amount rather than a growing one
    for (auto entry = state_.begin(); entry != state_.end();) {
        entry = entry->second.frame + retention < frame_ ? state_.erase(entry) : std::next(entry);
    }
}

void Immediate::block(const glm::vec2& min, const glm::vec2& max) {
    if (paint::inside(min, max, input_.cursor)) {
        // an id no widget answers to, so the block takes the cursor and gives it to nothing
        hovering_ = identify("##block");
    }
}

bool Immediate::capturing() const noexcept {
    // the held half matters as much as the hovered one: a scrubber being dragged keeps the
    // cursor after the drag has left its box, and a click that lands there is still spent
    return hovered_ != 0 || active_ != 0;
}

bool Immediate::press(std::string_view key, bool shifted, bool controlled) {
    if (!typing() || key.empty()) {
        return false;
    }
    if (!controlled && (key == "return" || key == "tab" || key == "escape")) {
        // the field gives the keyboard up when it is next drawn, after it has written what
        // was typed
        letGo_ = true;
        return true;
    }
    return input::edit(&line_, key, shifted, controlled, clipboard_);
}

bool Immediate::typed(std::string_view utf8) {
    if (!typing() || utf8.empty()) {
        return false;
    }
    // taken even when the limit refused it, so it never reaches the app's bindings
    line_.insert(utf8);
    return true;
}

bool Immediate::typing() const noexcept {
    return typing_ != 0 && !letGo_;
}

void Immediate::onTyping(const std::function<void(bool)>& changed) {
    onTyping_ = changed;
}

void Immediate::clipboard(const input::Clipboard& clipboard) {
    clipboard_ = clipboard;
}

void Immediate::focus(Id field) {
    const bool was = typing_ != 0;
    typing_ = field;
    letGo_ = false;
    if (field == 0) {
        written_.clear();
    }
    const bool now = typing_ != 0;
    if (was != now && onTyping_) {
        onTyping_(now);
    }
}

std::size_t Immediate::retained() const noexcept {
    return state_.size();
}

Immediate::Retained& Immediate::retain(Id id) {
    Retained& retained = state_[id];
    retained.frame = frame_;
    return retained;
}

Immediate::Id Immediate::identify(const std::string& label) const {
    std::uint64_t value = ids_.empty() ? hashSeed : ids_.back();
    for (const char letter : label) {
        value ^= static_cast<std::uint64_t>(static_cast<unsigned char>(letter));
        value *= hashPrime;
    }
    // an id of zero means "nothing", so a label that hashes to zero is given 1 instead
    return value == 0 ? 1 : value;
}

void Immediate::pushId(const std::string& id) {
    ids_.push_back(identify(id));
}

void Immediate::pushId(int id) {
    pushId(std::to_string(id));
}

void Immediate::popId() {
    if (!ids_.empty()) {
        ids_.pop_back();
    }
}

void Immediate::nextItemWidth(float width) {
    nextWidth_ = width;
}

float Immediate::itemWidth() {
    // measured from where the widget will be placed. On a shared row that is past the last
    // widget, and otherwise at the margin. place() has not run yet, so this works out the
    // same corner it will
    const float left = row_.sameLine ? row_.lastRight + dressing_.spacing : row_.margin;
    const float rest = std::max(row_.right - left, 0.0F);
    const float width = nextWidth_ > 0.0F ? std::min(nextWidth_, rest) : rest;
    nextWidth_ = 0.0F;
    return width;
}

glm::vec2 Immediate::place(const glm::vec2& size) {
    glm::vec2 corner;
    if (row_.sameLine) {
        corner = glm::vec2(row_.lastRight + dressing_.spacing, row_.top);
        row_.sameLine = false;
    } else {
        row_.top = row_.penY;
        row_.height = 0.0F;
        corner = glm::vec2(row_.margin, row_.top);
    }
    row_.lastRight = corner.x + size.x;
    row_.height = std::max(row_.height, size.y);
    row_.penY = row_.top + row_.height + dressing_.spacing;

    // an item that offers its box to the cursor claims it in interact(). One that does not,
    // such as text, is hovered when the window it is in holds the cursor, or when nothing
    // does outside a window
    itemMin_ = corner;
    itemMax_ = corner + size;
    itemOwner_ = window_.open ? window_.id : 0;
    return corner;
}

Immediate::Reaction Immediate::interact(Id id, const glm::vec2& min, const glm::vec2& max) {
    Reaction reaction;
    if (disabled_ > 0) {
        return reaction;
    }
    if (min == itemMin_ && max == itemMax_) {
        // the widget just placed is offering its own box, so it is what has to hold the
        // cursor for itemHovered(). A title bar or a scrollbar offers a box place() never
        // made, and leaves the last item alone
        itemOwner_ = id;
    }
    const bool within = paint::inside(min, max, input_.cursor);
    if (within) {
        hovering_ = id;
    }
    reaction.hovered = within && hovered_ == id;
    if (reaction.hovered && input_.pressed) {
        active_ = id;
        pressAt_ = input_.cursor;
    }
    reaction.held = active_ == id;
    reaction.clicked = reaction.held && reaction.hovered && input_.released;
    return reaction;
}

glm::vec4 Immediate::ink(const glm::vec4& colour) const {
    return disabled_ > 0 ? dressing_.disabledText : colour;
}

glm::vec4 Immediate::face(bool lit, bool hovered) const {
    if (disabled_ > 0) {
        return dressing_.widget;
    }
    if (lit) {
        return dressing_.highlight;
    }
    return hovered ? dressing_.hover : dressing_.widget;
}

void Immediate::label(const std::string& line, const glm::vec2& min, const glm::vec2& size,
    const glm::vec4& colour) const {
    // the baseline sits a quarter of a line below the box's middle, which is three quarters
    // of the way down a box one line tall. A font with ordinary descenders looks centred there
    const glm::vec2 pen(min.x, min.y + ((size.y + (dressing_.lineHeight * 0.5F)) * 0.5F));
    write_(line, pen, colour);
}

bool Immediate::window(const std::string& title, const v3d::type::geometry::Bound2D::Area& area, float alpha) {
    const glm::vec2& position = area.position;
    const glm::vec2& size = area.size;
    if (canvas_ == nullptr) {
        return false;
    }
    const Id id = identify(title);
    Retained& retained = retain(id);

    // the title bar both folds the window and moves it, told apart by whether the press
    // travelled. The move is applied before the window is placed, so a dragged
    // window is under the cursor on the frame it moved rather than the one after. The
    // pixels spent deciding are not applied, so a drag lags by the threshold once
    if (active_ != id) {
        retained.dragging = false;
    } else {
        if (!retained.dragging && glm::length(input_.cursor - pressAt_) > dragThreshold) {
            retained.dragging = true;
        }
        if (retained.dragging) {
            retained.offset += drag_;
        }
    }

    // the drag is kept as a displacement, not a position, so a window that its caller moves
    // every frame still follows the caller. The title bar is kept on the canvas, because the
    // bar is the only way to drag a window back
    const glm::vec2 room = glm::max(glm::vec2(static_cast<float>(canvas_->width()) - size.x,
        static_cast<float>(canvas_->height()) - dressing_.barHeight), glm::vec2(0.0F, 0.0F));
    const glm::vec2 min = glm::clamp(position + retained.offset, glm::vec2(0.0F, 0.0F), room);
    retained.offset = min - position;

    const glm::vec2 barMax(min.x + size.x, min.y + dressing_.barHeight);
    const glm::vec2 max = retained.collapsed ? barMax : min + size;

    glm::vec4 background = dressing_.panel;
    background.a *= std::clamp(alpha, 0.0F, 1.0F);
    paint::plateBox(canvas_, min, max, dressing_.radius, dressing_.borderWidth, {.inside = background, .outline = dressing_.border});
    paint::fillBox(canvas_, min + glm::vec2(dressing_.borderWidth, dressing_.borderWidth),
        glm::vec2(barMax.x - dressing_.borderWidth, barMax.y), dressing_.radius, dressing_.titleBar);
    label(title, min + glm::vec2(dressing_.padding, 0.0F), glm::vec2(size.x, dressing_.barHeight), dressing_.text);

    // a press that stayed put folds; one that moved has already moved the window this
    // frame and does not fold it as well
    const Reaction reaction = interact(id, min, barMax);
    if (reaction.clicked && !retained.dragging) {
        retained.collapsed = !retained.collapsed;
    }
    // a window takes the cursor from everything drawn before it, so a click meant for the
    // window on top does not reach what it covers. What goes in this one is drawn after
    // and takes it back
    if (paint::inside(min, max, input_.cursor)) {
        hovering_ = id;
    }

    window_.open = true;
    window_.margin = row_.margin;
    window_.right = row_.right;
    window_.id = id;
    window_.scroll = identify(title + " scrollbar");
    row_.margin = min.x + dressing_.padding;
    row_.right = max.x - dressing_.padding;
    row_.penY = barMax.y + dressing_.spacing;
    row_.sameLine = false;
    window_.clipped = false;
    window_.scrolls = false;
    if (retained.collapsed) {
        return false;
    }

    window_.bodyMin = glm::vec2(min.x + dressing_.borderWidth, barMax.y);
    window_.bodyMax = glm::vec2(max.x - dressing_.borderWidth, max.y - dressing_.borderWidth);
    window_.contentTop = barMax.y + dressing_.spacing;

    // whether there is a bar is decided by what the frame before this one drew, because how
    // tall the content is is only known once it has been drawn
    const float view = std::max(window_.bodyMax.y - window_.contentTop, 0.0F);
    window_.scrolls = retained.content > view;
    if (window_.scrolls) {
        row_.right -= dressing_.scrollbarWidth + dressing_.spacing;
        retained.scroll = std::clamp(retained.scroll, 0.0F, retained.content - view);
    } else {
        retained.scroll = 0.0F;
    }

    row_.penY = window_.contentTop - retained.scroll;
    canvas_->clip(window_.bodyMin, window_.bodyMax);
    window_.clipped = true;

    // the wheel turns the window the cursor is over, and a window drawn later is over one
    // drawn before it, so the last to claim the cursor keeps it
    if (paint::inside(min, max, input_.cursor)) {
        wheeled_ = id;
    }
    return true;
}

void Immediate::endWindow() {
    if (!window_.open) {
        return;
    }
    if (window_.clipped) {
        canvas_->unclip();
        window_.clipped = false;

        closeScroll({.id = window_.id, .scroll = window_.scroll, .contentTop = window_.contentTop, .bodyMax = window_.bodyMax},
            window_.scrolls ? Bar::Always : Bar::Never);
    }
    row_.margin = window_.margin;
    row_.right = window_.right;
    window_.open = false;
    window_.scrolls = false;
    row_.sameLine = false;
}

void Immediate::closeScroll(const Scrolled& region, Bar bar) {
    const Id id = region.id;
    const Id scroll = region.scroll;
    const float contentTop = region.contentTop;
    const glm::vec2& bodyMax = region.bodyMax;
    Retained& retained = retain(id);
    // how tall what was drawn came to. The pen has the scroll taken out of it and the gap
    // after the last row left in, so both go back before it is a height
    retained.content = std::max(row_.penY + retained.scroll - dressing_.spacing - contentTop, 0.0F);

    const float view = std::max(bodyMax.y - contentTop, 0.0F);
    const float span = std::max(retained.content - view, 0.0F);
    if (bar == Bar::Always || (bar == Bar::WhenNeeded && span > 0.0F)) {
        scrollbar(scroll, glm::vec2(bodyMax.x - dressing_.scrollbarWidth, contentTop), bodyMax, view, span,
            &retained.scroll);
    }
    if (wheeled_ == id && input_.wheel != 0.0F) {
        // a notch away from the reader shows what is above, which is a smaller offset
        retained.scroll = std::clamp(retained.scroll - (input_.wheel * dressing_.lineHeight * wheelRows), 0.0F, span);
    }
}

void Immediate::scrollbar(Id id, const glm::vec2& min, const glm::vec2& max, float view,
    float span, float* scroll) {
    const float track = max.y - min.y;
    if (track <= 0.0F || view <= 0.0F) {
        return;
    }

    const float length = component::thumbLength(track, view, view + span);

    const Reaction reaction = interact(id, min, max);
    if (reaction.held && track - length > 0.0F) {
        *scroll = component::dragOffset(track, length, input_.cursor.y - min.y, span);
    }

    glm::vec4 grip = dressing_.thumb;
    if (reaction.held) {
        grip = dressing_.highlight;
    } else if (reaction.hovered) {
        grip = dressing_.hover;
    }

    const float start = component::thumbStart({.track = track, .length = length, .offset = *scroll, .span = span});
    paint::fillBox(canvas_, min, max, dressing_.radius, dressing_.track);
    paint::fillBox(canvas_, glm::vec2(min.x, min.y + start), glm::vec2(max.x, min.y + start + length),
        dressing_.radius, grip);
}

void Immediate::text(const std::string& line) {
    if (canvas_ == nullptr) {
        return;
    }
    const glm::vec2 size(measure_(line), dressing_.lineHeight);
    label(line, place(size), size, ink(dressing_.text));
}

void Immediate::textDisabled(const std::string& line) {
    if (canvas_ == nullptr) {
        return;
    }
    const glm::vec2 size(measure_(line), dressing_.lineHeight);
    label(line, place(size), size, dressing_.disabledText);
}

void Immediate::textWrapped(const std::string& line) {
    if (canvas_ == nullptr) {
        return;
    }
    for (const std::string& row : paint::wrap(line, row_.right - row_.margin, measure_)) {
        text(row);
    }
}

void Immediate::bulletText(const std::string& line) {
    if (canvas_ == nullptr) {
        return;
    }
    const glm::vec2 size(dressing_.indent + measure_(line), dressing_.lineHeight);
    const glm::vec2 corner = place(size);
    const float radius = dressing_.lineHeight * bulletScale * 0.5F;
    canvas_->circle(glm::vec2(corner.x + (dressing_.indent * 0.5F), corner.y + (dressing_.lineHeight * 0.5F)),
        radius, bulletSides, ink(dressing_.text));
    label(line, corner + glm::vec2(dressing_.indent, 0.0F), glm::vec2(size.x, size.y), ink(dressing_.text));
}

bool Immediate::button(const std::string& label) {
    if (canvas_ == nullptr) {
        return false;
    }
    const glm::vec2 size(measure_(label) + (dressing_.padding * 2.0F), dressing_.barHeight);
    const glm::vec2 min = place(size);
    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);

    paint::fillBox(canvas_, min, min + size, dressing_.radius, face(reaction.held, reaction.hovered));
    this->label(label, min + glm::vec2(dressing_.padding, 0.0F), size,
        ink(reaction.hovered ? dressing_.activeText : dressing_.text));
    return reaction.clicked;
}

bool Immediate::smallButton(const std::string& label) {
    if (canvas_ == nullptr) {
        return false;
    }
    const glm::vec2 size(measure_(label) + dressing_.padding, dressing_.lineHeight);
    const glm::vec2 min = place(size);
    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);

    paint::fillBox(canvas_, min, min + size, dressing_.radius, face(reaction.held, reaction.hovered));
    this->label(label, min + glm::vec2(dressing_.padding * 0.5F, 0.0F), size,
        ink(reaction.hovered ? dressing_.activeText : dressing_.text));
    return reaction.clicked;
}

bool Immediate::selectable(const std::string& label, bool selected) {
    if (canvas_ == nullptr) {
        return false;
    }
    const glm::vec2 size(itemWidth(), dressing_.lineHeight);
    const glm::vec2 min = place(size);
    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);

    if (selected) {
        paint::fillBox(canvas_, min, min + size, 0.0F, dressing_.highlight);
    } else if (reaction.hovered) {
        paint::fillBox(canvas_, min, min + size, 0.0F, dressing_.hover);
    }
    this->label(label, min + glm::vec2(dressing_.padding * 0.5F, 0.0F), size,
        ink(selected ? dressing_.activeText : dressing_.text));
    return reaction.clicked;
}

bool Immediate::dragInt(const std::string& label, int* value, int low, int high) {
    if (canvas_ == nullptr || value == nullptr) {
        return false;
    }
    const glm::vec2 size(itemWidth(), dressing_.barHeight);
    const glm::vec2 min = place(size);
    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);

    bool changed = false;
    if (reaction.held && drag_.x != 0.0F) {
        const int moved = static_cast<int>(std::lround(drag_.x * scrubRate));
        if (moved != 0) {
            const int wanted = std::clamp(*value + moved, low, high);
            changed = wanted != *value;
            *value = wanted;
        }
    }

    paint::fillBox(canvas_, min, min + size, dressing_.radius, face(reaction.held, reaction.hovered));
    std::string shown(label);
    shown.append("  ").append(std::to_string(*value));
    this->label(shown, min + glm::vec2(dressing_.padding, 0.0F), size, ink(dressing_.text));
    return changed;
}

bool Immediate::textField(const std::string& label, std::string* value, std::size_t limit) {
    if (canvas_ == nullptr || value == nullptr) {
        return false;
    }
    const glm::vec2 size(itemWidth(), dressing_.barHeight);
    const glm::vec2 min = place(size);
    const glm::vec2 max = min + size;
    const Id id = identify(label);
    const Reaction reaction = interact(id, min, max);

    const glm::vec2 low(min.x + dressing_.padding, min.y + dressing_.borderWidth);
    const glm::vec2 high(max.x - dressing_.padding, max.y - dressing_.borderWidth);
    const float room = std::max(high.x - low.x, 0.0F);

    if (typing_ == id && *value != written_) {
        // the caller changed the string while it was being edited, and the caller's string
        // is the value, so the edit starts again from it
        line_.text(*value);
        written_ = *value;
    }
    if (reaction.hovered && input_.pressed) {
        if (typing_ != id) {
            focus(id);
            line_.text(*value);
            written_ = *value;
        }
        // the press is measured against the line as it was drawn, slid as far as the caret
        // had pushed it
        const float caret = measure_(line_.text().substr(0, line_.caret()));
        const float slid = std::max(caret - room, 0.0F);
        line_.caret(line_.at(input_.cursor.x - (low.x - slid), measure_));
    }

    bool changed = false;
    if (typing_ == id) {
        typingDrawn_ = true;
        line_.limit(limit);
        if (line_.text() != *value) {
            *value = std::string(line_.text());
            written_ = *value;
            changed = true;
        }
        // a press anywhere else, a disabled field, and a return, tab or escape all give the
        // keyboard up. What was typed has been written by then
        if (letGo_ || disabled_ > 0 || (input_.pressed && !reaction.hovered)) {
            focus(0);
        }
    }

    const bool focused = typing_ == id;
    paint::plateBox(canvas_, min, max, dressing_.radius, dressing_.borderWidth,
        {.inside = face(false, reaction.hovered && !focused), .outline = focused ? dressing_.focus : dressing_.border});
    if (high.x <= low.x) {
        return changed;
    }
    canvas_->clip(low, high);
    if (!focused && value->empty()) {
        this->label(label, low, glm::vec2(room, size.y - (dressing_.borderWidth * 2.0F)), dressing_.placeholder);
        canvas_->unclip();
        return changed;
    }

    // the line slides left when the caret would be past the far edge, so a full field can
    // still be typed into
    const std::string_view line = focused ? line_.text() : std::string_view(*value);
    const float caret = focused ? measure_(line.substr(0, line_.caret())) : 0.0F;
    const float pen = low.x - std::max(caret - room, 0.0F);
    if (focused && line_.selected()) {
        // the highlight goes behind the line, which is drawn whole in one colour, so the
        // runs either side of it cannot measure differently from the prefix that places the
        // caret
        const float from = measure_(line.substr(0, std::min(line_.anchor(), line_.caret())));
        const float to = measure_(line.substr(0, std::max(line_.anchor(), line_.caret())));
        paint::fillBox(canvas_, glm::vec2(pen + from, low.y), glm::vec2(pen + to, high.y), 0.0F, dressing_.highlight);
    }
    this->label(std::string(line), glm::vec2(pen, low.y), glm::vec2(room, high.y - low.y), ink(dressing_.text));
    if (focused) {
        const float stem = std::max(dressing_.borderWidth, 1.0F);
        canvas_->rect(glm::vec2(pen + caret, low.y), glm::vec2(pen + caret + stem, high.y), dressing_.caret);
    }
    canvas_->unclip();
    return changed;
}

bool Immediate::checkbox(const std::string& label, bool* value) {
    if (canvas_ == nullptr || value == nullptr) {
        return false;
    }
    const float side = dressing_.lineHeight;
    const glm::vec2 size(side + dressing_.padding + measure_(label), side);
    const glm::vec2 min = place(size);
    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);
    if (reaction.clicked) {
        *value = !*value;
    }

    paint::fillBox(canvas_, min, min + glm::vec2(side, side), dressing_.radius, face(reaction.held, reaction.hovered));
    if (*value) {
        const glm::vec2 inset(side * tickInset, side * tickInset);
        paint::fillBox(canvas_, min + inset, min + glm::vec2(side, side) - inset, 0.0F, ink(dressing_.mark));
    }
    this->label(label, min + glm::vec2(side + dressing_.padding, 0.0F), size,
        ink(reaction.hovered ? dressing_.activeText : dressing_.text));
    return reaction.clicked;
}

bool Immediate::sliderFloat(const std::string& label, float* value, float low, float high) {
    if (canvas_ == nullptr || value == nullptr) {
        return false;
    }
    const glm::vec2 size(itemWidth(), dressing_.barHeight);
    const glm::vec2 min = place(size);
    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);

    // the value is where the cursor is along the track, so a press jumps to it and a drag
    // follows it, and a drag past either end holds the value at that end
    bool changed = false;
    if (reaction.held && size.x > 0.0F) {
        const float along = std::clamp((input_.cursor.x - min.x) / size.x, 0.0F, 1.0F);
        const float wanted = low + ((high - low) * along);
        changed = wanted != *value;
        *value = wanted;
    }

    const float span = high - low;
    const float part = span == 0.0F ? 0.0F : std::clamp((*value - low) / span, 0.0F, 1.0F);
    paint::fillBox(canvas_, min, min + size, dressing_.radius, face(reaction.held, reaction.hovered));
    if (part > 0.0F) {
        paint::fillBox(canvas_, min, glm::vec2(min.x + (size.x * part), min.y + size.y), dressing_.radius, ink(dressing_.fill));
    }
    this->label(std::format("{}  {:.2f}", label, *value), min + glm::vec2(dressing_.padding, 0.0F), size, ink(dressing_.text));
    return changed;
}

bool Immediate::combo(const std::string& label, int* selected, const std::vector<std::string>& items) {
    if (canvas_ == nullptr || selected == nullptr) {
        return false;
    }
    const glm::vec2 size(itemWidth(), dressing_.barHeight);
    const glm::vec2 min = place(size);
    const glm::vec2 max = min + size;
    const Id id = identify(label);
    Retained& retained = retain(id);

    // a click on the list last frame is taken now, by the call that holds the caller's index
    bool changed = false;
    if (retained.chosen >= 0) {
        if (static_cast<std::size_t>(retained.chosen) < items.size() && retained.chosen != *selected) {
            *selected = retained.chosen;
            changed = true;
        }
        retained.chosen = -1;
    }

    const Reaction reaction = interact(id, min, max);
    if (reaction.clicked) {
        popup_.id = popup_.id == id ? 0 : id;
    }
    const bool open = popup_.id == id;
    if (open) {
        popup_.asked = true;
        popup_.boxMin = min;
        popup_.boxMax = max;
        popup_.items = items;
        popup_.selected = *selected;
    }

    paint::fillBox(canvas_, min, max, dressing_.radius, face(open || reaction.held, reaction.hovered));
    const float mark = dressing_.lineHeight * comboMark;
    const glm::vec2 markMin(max.x - dressing_.padding - mark, min.y + ((size.y - mark) * 0.5F));
    paint::fillBox(canvas_, markMin, markMin + glm::vec2(mark, mark), 0.0F,
        ink(reaction.hovered ? dressing_.activeText : dressing_.text));
    if (*selected >= 0 && static_cast<std::size_t>(*selected) < items.size()) {
        this->label(items[static_cast<std::size_t>(*selected)], min + glm::vec2(dressing_.padding, 0.0F), size,
            ink(reaction.hovered ? dressing_.activeText : dressing_.text));
    }
    return changed;
}

void Immediate::popup() {
    if (popup_.id == 0) {
        return;
    }
    if (!popup_.asked || canvas_ == nullptr) {
        // a combo that was not drawn this frame cannot hold a list open
        popup_.id = 0;
        return;
    }
    const float row = dressing_.lineHeight;
    const glm::vec2 min(popup_.boxMin.x, popup_.boxMax.y);
    const glm::vec2 max(popup_.boxMax.x, min.y + (row * static_cast<float>(popup_.items.size())));
    paint::plateBox(canvas_, min, max, 0.0F, dressing_.borderWidth, {.inside = dressing_.panel, .outline = dressing_.border});
    if (paint::inside(min, max, input_.cursor)) {
        // the list takes the cursor from what it covers, so a press between two items does
        // not reach a widget under it
        hovering_ = popup_.id;
    }

    // the items' ids hang off the combo's, so two combos' lists do not share them
    ids_.push_back(popup_.id);
    bool close = false;
    for (std::size_t index = 0; index < popup_.items.size(); index++) {
        const glm::vec2 itemMin(min.x, min.y + (row * static_cast<float>(index)));
        const glm::vec2 itemMax(max.x, itemMin.y + row);
        const Reaction reaction = interact(identify(std::to_string(index)), itemMin, itemMax);
        const bool chosen = static_cast<int>(index) == popup_.selected;
        if (chosen || reaction.hovered) {
            paint::fillBox(canvas_, itemMin, itemMax, 0.0F, chosen ? dressing_.highlight : dressing_.hover);
        }
        label(popup_.items[index], itemMin + glm::vec2(dressing_.padding, 0.0F), glm::vec2(max.x - min.x, row),
            chosen || reaction.hovered ? dressing_.activeText : dressing_.text);
        if (reaction.clicked) {
            retain(popup_.id).chosen = static_cast<int>(index);
            close = true;
        }
    }
    ids_.pop_back();

    // a press outside the list closes it. One on the combo's own box is left to the combo,
    // whose click toggles it
    if (input_.pressed && !paint::inside(min, max, input_.cursor) &&
        !paint::inside(popup_.boxMin, popup_.boxMax, input_.cursor)) {
        close = true;
    }
    if (close) {
        popup_.id = 0;
    }
}

bool Immediate::itemHovered() const noexcept {
    return paint::inside(itemMin_, itemMax_, input_.cursor) && hovered_ == itemOwner_;
}

void Immediate::progressBar(float fraction, const std::string& overlay) {
    if (canvas_ == nullptr) {
        return;
    }
    const glm::vec2 size(itemWidth(), dressing_.lineHeight);
    const glm::vec2 min = place(size);
    const glm::vec2 max = min + size;

    paint::fillBox(canvas_, min, max, dressing_.radius, dressing_.track);
    const float part = std::clamp(fraction, 0.0F, 1.0F);
    if (part > 0.0F) {
        paint::fillBox(canvas_, min, glm::vec2(min.x + (size.x * part), max.y), dressing_.radius, ink(dressing_.fill));
    }
    if (!overlay.empty()) {
        const glm::vec2 corner(min.x + ((size.x - measure_(overlay)) * 0.5F), min.y);
        label(overlay, corner, size, ink(dressing_.activeText));
    }
}

void Immediate::separator() {
    if (canvas_ == nullptr) {
        return;
    }
    const glm::vec2 size(row_.right - row_.margin, ruleWidth + dressing_.spacing);
    const glm::vec2 min = place(size);
    const float middle = min.y + (dressing_.spacing * 0.5F);
    canvas_->rect(glm::vec2(min.x, middle), glm::vec2(min.x + size.x, middle + ruleWidth), dressing_.rule);
}

void Immediate::sameLine() {
    row_.sameLine = true;
}

void Immediate::beginDisabled() {
    disabled_++;
}

void Immediate::endDisabled() {
    if (disabled_ > 0) {
        disabled_--;
    }
}

bool Immediate::tabBar(const std::string& id) {
    if (canvas_ == nullptr) {
        return false;
    }
    tabs_.id = identify(id);
    tabs_.open = true;
    tabs_.index = 0;
    tabs_.changed = false;
    tabs_.taken = false;
    // the strip takes a row of its own and the pen goes past it, so what a selected tab
    // holds is drawn under the whole strip rather than beside the next tab
    tabs_.top = row_.penY;
    tabs_.pen = row_.margin;
    row_.penY = tabs_.top + dressing_.barHeight;
    canvas_->rect(glm::vec2(row_.margin, row_.penY), glm::vec2(row_.right, row_.penY + ruleWidth), dressing_.rule);
    row_.penY += ruleWidth + dressing_.spacing;
    row_.sameLine = false;
    return true;
}

bool Immediate::tab(const std::string& label) {
    if (canvas_ == nullptr || !tabs_.open) {
        return false;
    }
    const Retained& retained = retain(tabs_.id);
    const unsigned int index = tabs_.index++;
    const glm::vec2 size(measure_(label) + (dressing_.padding * 2.0F), dressing_.barHeight);
    const glm::vec2 min(tabs_.pen, tabs_.top);
    tabs_.pen += size.x + dressing_.spacing;

    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);
    if (reaction.clicked) {
        tabs_.wanted = index;
        tabs_.changed = true;
    }
    const bool selected = retained.tab == index;
    tabs_.taken = tabs_.taken || selected;

    // a tab at rest is drawn as a tab, not as a widget's face
    const bool lit = disabled_ == 0 && (selected || reaction.hovered);
    paint::fillBox(canvas_, min, min + size, dressing_.radius, lit ? face(selected, reaction.hovered) : dressing_.tab);
    this->label(label, min + glm::vec2(dressing_.padding, 0.0F), size,
        selected ? dressing_.activeText : dressing_.text);
    return selected;
}

void Immediate::endTabBar() {
    if (!tabs_.open) {
        return;
    }
    if (tabs_.changed) {
        retain(tabs_.id).tab = tabs_.wanted;
    } else if (!tabs_.taken && tabs_.index > 0) {
        // a strip whose selected tab is no longer there falls back to the first, so a bar
        // is never drawn with nothing chosen
        retain(tabs_.id).tab = 0;
    }
    tabs_.open = false;
}

bool Immediate::table(const std::string& id, const TableShape& shape) {
    const unsigned int columns = shape.columns;
    const float height = shape.height;
    if (canvas_ == nullptr || columns == 0) {
        return false;
    }
    table_.id = identify(id);
    pushId(id);
    table_.open = true;
    table_.headers.clear();
    table_.widths.assign(columns, 0.0F);
    table_.left = row_.margin;
    table_.right = row_.right;
    table_.column = 0;
    table_.height = std::max(height, 0.0F);
    table_.top = row_.penY;
    table_.contentTop = row_.penY;
    table_.clipped = false;
    table_.scroll = identify("scrollbar");
    row_.sameLine = false;

    // the gutter is reserved for as long as the table has a height, whether or not there
    // is anything to scroll yet. The columns then do not re-flow when a row arrives. The
    // bar can also be drawn the frame the content overflows rather than the one after, as
    // a window's has to be
    if (table_.height > 0.0F) {
        row_.right -= dressing_.scrollbarWidth + dressing_.spacing;
    }
    return true;
}

void Immediate::tableBody() {
    if (!table_.open || table_.height <= 0.0F || table_.clipped) {
        return;
    }
    Retained& retained = retain(table_.id);
    table_.contentTop = row_.penY;

    const glm::vec2 min(table_.left, table_.contentTop);
    const glm::vec2 max(table_.right, table_.top + table_.height);
    const float view = std::max(max.y - min.y, 0.0F);
    retained.scroll = std::clamp(retained.scroll, 0.0F, std::max(retained.content - view, 0.0F));

    canvas_->clip(min, max);
    table_.clipped = true;
    row_.penY = table_.contentTop - retained.scroll;

    // the wheel turns the innermost region under the cursor. A table takes it from the
    // window it is drawn in, by the same rule that lets a window take it from one under it
    if (paint::inside(glm::vec2(table_.left, table_.top), max, input_.cursor)) {
        wheeled_ = table_.id;
    }
}

void Immediate::column(const std::string& label, float width) {
    if (!table_.open || table_.headers.size() >= table_.widths.size()) {
        return;
    }
    table_.widths[table_.headers.size()] = width;
    table_.headers.push_back(label);
}

float Immediate::columnStart(unsigned int index) const {
    // a column given no width of its own shares out what the named ones left
    float named = 0.0F;
    unsigned int unnamed = 0;
    for (const float width : table_.widths) {
        if (width > 0.0F) {
            named += width;
        } else {
            unnamed++;
        }
    }
    const float spare = unnamed == 0 ? 0.0F
        : std::max(0.0F, (row_.right - table_.left - named) / static_cast<float>(unnamed));

    float start = table_.left;
    for (unsigned int column = 0; column < index && column < table_.widths.size(); column++) {
        start += table_.widths[column] > 0.0F ? table_.widths[column] : spare;
    }
    return start;
}

void Immediate::headerRow() {
    if (canvas_ == nullptr || !table_.open) {
        return;
    }
    // the band runs the whole width of the table, not the width its columns were given. A
    // scrolling table's gutter is banded beside the names, not notched out of the band. The
    // two widths are the same for a table that does not scroll
    const glm::vec2 min(table_.left, row_.penY);
    const glm::vec2 max(table_.right, row_.penY + dressing_.lineHeight);
    paint::fillBox(canvas_, min, max, 0.0F, dressing_.titleBar);
    for (std::size_t index = 0; index < table_.headers.size(); index++) {
        const glm::vec2 corner(columnStart(static_cast<unsigned int>(index)) + (dressing_.padding * 0.5F), min.y);
        label(table_.headers[index], corner, glm::vec2(0.0F, dressing_.lineHeight), dressing_.disabledText);
    }
    row_.penY = max.y + dressing_.spacing;
    nextRow();
}

void Immediate::nextRow() {
    if (!table_.open) {
        return;
    }
    // the scrolled region holds the rows, so it starts at the first row: below the header
    // when there is one, and at the top of the table when there is not
    tableBody();
    table_.column = 0;
    row_.margin = columnStart(0);
    row_.sameLine = false;
}

void Immediate::nextColumn() {
    if (!table_.open) {
        return;
    }
    table_.column++;
    // sameLine() keeps the next widget on the row. Setting lastRight puts it at the column's
    // start, not wherever the last widget ended
    row_.sameLine = true;
    row_.lastRight = columnStart(table_.column) - dressing_.spacing;
}

void Immediate::endTable() {
    if (!table_.open) {
        return;
    }
    if (table_.clipped) {
        canvas_->unclip();
        table_.clipped = false;

        const float bottom = table_.top + table_.height;
        closeScroll({.id = table_.id, .scroll = table_.scroll, .contentTop = table_.contentTop, .bodyMax = glm::vec2(table_.right, bottom)},
            Bar::WhenNeeded);
        // what follows the table goes under the region, not under the rows, because the rows
        // can be shorter or taller than the height the table was given
        row_.penY = bottom + dressing_.spacing;
    }
    table_.open = false;
    row_.margin = table_.left;
    row_.right = table_.right;
    row_.sameLine = false;
    popId();
}

};  // namespace v3d::ui
