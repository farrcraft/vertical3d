/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Immediate.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "Painter.h"
#include "Style.h"
#include "component/Scrollbar.h"
#include "style/Theme.h"

#include "../render/realtime/Canvas.h"

namespace v3d::ui {

namespace {

/**
 * The seed and the multiplier of FNV-1a, which is what a label is turned into an id with.
 * Any hash would do; this one is four lines and needs no table.
 **/
const std::uint64_t hashSeed = 1469598103934665603ULL;
const std::uint64_t hashPrime = 1099511628211ULL;

/**
 * How thick a separator and the line under a tab strip are.
 **/
const float ruleWidth = 1.0f;

/**
 * How far a scrub moves the value for each pixel the cursor travels. One unit per pixel is
 * too fast to land on a number by hand and too slow to cross a wide range, and a quarter
 * is the compromise every tool of this kind settles on.
 **/
const float scrubRate = 0.25f;

/**
 * How far one notch of the wheel scrolls a window, as a multiple of a row. Three rows is
 * what a desktop scrolls by, and reads as a deliberate move rather than a nudge.
 **/
const float wheelRows = 3.0f;

/**
 * The side of the square a bullet is drawn in, as a fraction of a row.
 **/
const float bulletScale = 0.28f;

/**
 * How many segments a bullet is drawn with.
 **/
const unsigned int bulletSides = 8;

/**
 * Break a line at the spaces that do not fit a width.
 *
 * Greedy: a word goes on the current line if it fits, and starts a new one if it does not.
 * A single word wider than the whole width is left over the edge rather than split, which
 * is the lesser of the two wrong answers for a name or a path.
 **/
std::vector<std::string> wrap(const std::string& line, float width,
    const std::function<float(const std::string&)>& measure) {
    std::vector<std::string> rows;
    std::istringstream words(line);
    std::string word;
    std::string row;
    while (words >> word) {
        if (row.empty()) {
            row = word;
            continue;
        }
        std::string wider(row);
        wider.append(" ").append(word);
        if (measure(wider) <= width) {
            row = wider;
        } else {
            rows.push_back(row);
            row = word;
        }
    }
    if (!row.empty()) {
        rows.push_back(row);
    }
    return rows;
}

};  // namespace

Immediate::Input::Input() noexcept :
cursor(0.0f, 0.0f),
down(false),
pressed(false),
released(false),
wheel(0.0f) {
}

Immediate::Reaction::Reaction() noexcept :
hovered(false),
held(false),
clicked(false) {
}

Immediate::Retained::Retained() noexcept :
frame(0),
tab(0),
scroll(0.0f),
content(0.0f),
collapsed(false) {
}

Immediate::Dressing::Dressing() noexcept :
lineHeight(18.0f),
padding(6.0f),
spacing(4.0f),
barHeight(22.0f),
borderWidth(1.0f),
radius(3.0f),
indent(14.0f),
scrollbarWidth(10.0f),
panel(0.05f, 0.06f, 0.09f, 0.92f),
border(0.35f, 0.38f, 0.45f, 1.0f),
titleBar(0.12f, 0.14f, 0.19f, 1.0f),
text(0.78f, 0.80f, 0.84f, 1.0f),
activeText(1.0f, 1.0f, 1.0f, 1.0f),
dimText(0.45f, 0.47f, 0.52f, 1.0f),
widget(0.16f, 0.18f, 0.24f, 1.0f),
highlight(0.16f, 0.34f, 0.58f, 1.0f),
hover(0.22f, 0.25f, 0.33f, 1.0f),
fill(0.30f, 0.62f, 0.36f, 1.0f),
rule(0.24f, 0.26f, 0.32f, 1.0f) {
}

Immediate::Immediate(const Measure& measure, const Write& write) :
    measure_(measure),
    write_(write),
    canvas_(nullptr),
    previousCursor_(0.0f, 0.0f),
    drag_(0.0f, 0.0f),
    hovered_(0),
    hovering_(0),
    active_(0),
    frame_(0),
    margin_(0.0f),
    right_(0.0f),
    penY_(0.0f),
    rowTop_(0.0f),
    rowHeight_(0.0f),
    lastRight_(0.0f),
    sameLine_(false),
    disabled_(0),
    inWindow_(false),
    windowMargin_(0.0f),
    windowRight_(0.0f),
    window_(0),
    windowScroll_(0),
    bodyMin_(0.0f, 0.0f),
    bodyMax_(0.0f, 0.0f),
    contentTop_(0.0f),
    windowScrolls_(false),
    windowClipped_(false),
    wheeled_(0),
    tabBar_(0),
    inTabBar_(false),
    tabPen_(0.0f),
    tabTop_(0.0f),
    tabIndex_(0),
    tabWanted_(0),
    tabChanged_(false),
    tabTaken_(false),
    inTable_(false),
    tableLeft_(0.0f),
    columnIndex_(0) {
}

// out of line, so that the header need not complete the types the members hold
Immediate::~Immediate() {
}

Immediate::Dressing& Immediate::dressing() noexcept {
    return dressing_;
}

void Immediate::theme(const boost::shared_ptr<style::Theme>& theme) {
    theme_ = theme;
    if (!theme_) {
        return;
    }
    const std::vector<boost::shared_ptr<v3d::ui::Style>> styles = theme_->getStyleSet(std::string(), "ui");
    if (styles.empty()) {
        return;
    }
    const boost::shared_ptr<v3d::ui::Style>& chrome = styles.front();

    readColour(chrome, "panel", &dressing_.panel);
    readColour(chrome, "border", &dressing_.border);
    readColour(chrome, "title-bar", &dressing_.titleBar);
    readColour(chrome, "text", &dressing_.text);
    readColour(chrome, "active-text", &dressing_.activeText);
    readColour(chrome, "dim-text", &dressing_.dimText);
    readColour(chrome, "widget", &dressing_.widget);
    readColour(chrome, "highlight", &dressing_.highlight);
    readColour(chrome, "hover", &dressing_.hover);
    readColour(chrome, "fill", &dressing_.fill);
    readColour(chrome, "rule", &dressing_.rule);

    readMetric(chrome, "line-height", &dressing_.lineHeight);
    readMetric(chrome, "padding", &dressing_.padding);
    readMetric(chrome, "spacing", &dressing_.spacing);
    readMetric(chrome, "bar-height", &dressing_.barHeight);
    readMetric(chrome, "border-width", &dressing_.borderWidth);
    readMetric(chrome, "radius", &dressing_.radius);
    readMetric(chrome, "scrollbar-width", &dressing_.scrollbarWidth);
}

void Immediate::begin(v3d::render::realtime::Canvas* canvas, const Input& input) {
    canvas_ = canvas;
    input_ = input;
    frame_++;
    drag_ = input.cursor - previousCursor_;
    hovering_ = 0;
    ids_.clear();
    disabled_ = 0;
    inWindow_ = false;
    windowClipped_ = false;
    wheeled_ = 0;
    inTabBar_ = false;
    inTable_ = false;
    sameLine_ = false;
    margin_ = 0.0f;
    penY_ = 0.0f;
    right_ = canvas == nullptr ? 0.0f : static_cast<float>(canvas->width());
}

void Immediate::end() {
    // what the cursor was found on this frame is what answers the next one, which is how a
    // window drawn later takes the cursor from one under it
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
    // an id of zero is what "nothing" is, so the one label that hashes to it moves along
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

glm::vec2 Immediate::place(const glm::vec2& size) {
    glm::vec2 corner;
    if (sameLine_) {
        corner = glm::vec2(lastRight_ + dressing_.spacing, rowTop_);
        sameLine_ = false;
    } else {
        rowTop_ = penY_;
        rowHeight_ = 0.0f;
        corner = glm::vec2(margin_, rowTop_);
    }
    lastRight_ = corner.x + size.x;
    rowHeight_ = std::max(rowHeight_, size.y);
    penY_ = rowTop_ + rowHeight_ + dressing_.spacing;
    return corner;
}

Immediate::Reaction Immediate::interact(Id id, const glm::vec2& min, const glm::vec2& max) {
    Reaction reaction;
    if (disabled_ > 0) {
        return reaction;
    }
    const bool within = inside(min, max, input_.cursor);
    if (within) {
        hovering_ = id;
    }
    reaction.hovered = within && hovered_ == id;
    if (reaction.hovered && input_.pressed) {
        active_ = id;
    }
    reaction.held = active_ == id;
    reaction.clicked = reaction.held && reaction.hovered && input_.released;
    return reaction;
}

glm::vec4 Immediate::ink(const glm::vec4& colour) const {
    return disabled_ > 0 ? dressing_.dimText : colour;
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
    // the baseline sits three quarters of the way down a row, which is where a font with
    // ordinary descenders looks centred
    const glm::vec2 pen(min.x, min.y + (size.y + dressing_.lineHeight * 0.5f) * 0.5f);
    write_(line, pen, colour);
}

bool Immediate::window(const std::string& title, const glm::vec2& position, const glm::vec2& size,
    float alpha) {
    if (canvas_ == nullptr) {
        return false;
    }
    const Id id = identify(title);
    Retained& retained = retain(id);

    const glm::vec2 min = position;
    const glm::vec2 barMax(position.x + size.x, position.y + dressing_.barHeight);
    const glm::vec2 max = retained.collapsed ? barMax : position + size;

    glm::vec4 background = dressing_.panel;
    background.a *= std::clamp(alpha, 0.0f, 1.0f);
    plateBox(canvas_, min, max, dressing_.radius, dressing_.borderWidth, background, dressing_.border);
    fillBox(canvas_, min + glm::vec2(dressing_.borderWidth, dressing_.borderWidth),
        glm::vec2(barMax.x - dressing_.borderWidth, barMax.y), dressing_.radius, dressing_.titleBar);
    label(title, min + glm::vec2(dressing_.padding, 0.0f), glm::vec2(size.x, dressing_.barHeight), dressing_.text);

    // the title bar is what folds the window away, which is the only thing this layer lets
    // a window be dragged or resized by
    const Reaction reaction = interact(id, min, barMax);
    if (reaction.clicked) {
        retained.collapsed = !retained.collapsed;
    }
    // a window takes the cursor from everything drawn before it, so a click meant for the
    // window on top does not reach what it covers. What goes in this one is drawn after
    // and takes it back
    if (inside(min, max, input_.cursor)) {
        hovering_ = id;
    }

    inWindow_ = true;
    windowMargin_ = margin_;
    windowRight_ = right_;
    window_ = id;
    windowScroll_ = identify(title + " scrollbar");
    margin_ = min.x + dressing_.padding;
    right_ = max.x - dressing_.padding;
    penY_ = barMax.y + dressing_.spacing;
    sameLine_ = false;
    windowClipped_ = false;
    windowScrolls_ = false;
    if (retained.collapsed) {
        return false;
    }

    bodyMin_ = glm::vec2(min.x + dressing_.borderWidth, barMax.y);
    bodyMax_ = glm::vec2(max.x - dressing_.borderWidth, max.y - dressing_.borderWidth);
    contentTop_ = barMax.y + dressing_.spacing;

    // whether there is a bar is decided by what the frame before this one drew, because how
    // tall the content is is only known once it has been drawn
    const float view = std::max(bodyMax_.y - contentTop_, 0.0f);
    windowScrolls_ = retained.content > view;
    if (windowScrolls_) {
        right_ -= dressing_.scrollbarWidth + dressing_.spacing;
        retained.scroll = std::clamp(retained.scroll, 0.0f, retained.content - view);
    } else {
        retained.scroll = 0.0f;
    }

    penY_ = contentTop_ - retained.scroll;
    canvas_->clip(bodyMin_, bodyMax_);
    windowClipped_ = true;

    // the wheel turns the window the cursor is over, and a window drawn later is over one
    // drawn before it, so the last to claim the cursor keeps it
    if (inside(min, max, input_.cursor)) {
        wheeled_ = id;
    }
    return true;
}

void Immediate::endWindow() {
    if (!inWindow_) {
        return;
    }
    if (windowClipped_) {
        canvas_->unclip();
        windowClipped_ = false;

        Retained& retained = retain(window_);
        // how tall what was drawn came to. The pen has the scroll taken out of it and the
        // gap after the last row left in, so both go back before it is a height
        retained.content = std::max(penY_ + retained.scroll - dressing_.spacing - contentTop_, 0.0f);

        const float view = std::max(bodyMax_.y - contentTop_, 0.0f);
        const float span = std::max(retained.content - view, 0.0f);
        if (windowScrolls_) {
            scrollbar(view, span, &retained.scroll);
        }
        if (wheeled_ == window_ && input_.wheel != 0.0f) {
            // a notch away from the reader shows what is above, which is a smaller offset
            retained.scroll = std::clamp(retained.scroll - input_.wheel * dressing_.lineHeight * wheelRows,
                0.0f, span);
        }
    }
    margin_ = windowMargin_;
    right_ = windowRight_;
    inWindow_ = false;
    windowScrolls_ = false;
    sameLine_ = false;
}

void Immediate::scrollbar(float view, float span, float* scroll) {
    const glm::vec2 min(bodyMax_.x - dressing_.scrollbarWidth, contentTop_);
    const glm::vec2 max(bodyMax_.x, bodyMax_.y);
    const float track = max.y - min.y;
    if (track <= 0.0f || view <= 0.0f) {
        return;
    }

    // as much of the track as the window shows of its content, so the thumb reads as how
    // much there is as well as where in it the window is
    const float length = std::clamp(track * (view / (view + span)),
        std::min(component::Scrollbar::minimumThumb, track), track);
    const float room = track - length;

    const Reaction reaction = interact(windowScroll_, min, max);
    if (reaction.held && room > 0.0f) {
        // the cursor holds the middle of the thumb, so what is under it stays under it
        *scroll = span * std::clamp((input_.cursor.y - min.y - length * 0.5f) / room, 0.0f, 1.0f);
    }

    // the thumb rests in the rule colour rather than the widget one, so that it reads
    // against the track under it
    glm::vec4 grip = dressing_.rule;
    if (reaction.held) {
        grip = dressing_.highlight;
    } else if (reaction.hovered) {
        grip = dressing_.hover;
    }

    const float start = span > 0.0f ? room * (*scroll / span) : 0.0f;
    fillBox(canvas_, min, max, dressing_.radius, dressing_.widget);
    fillBox(canvas_, glm::vec2(min.x, min.y + start), glm::vec2(max.x, min.y + start + length),
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
    label(line, place(size), size, dressing_.dimText);
}

void Immediate::textWrapped(const std::string& line) {
    if (canvas_ == nullptr) {
        return;
    }
    for (const std::string& row : wrap(line, right_ - margin_, measure_)) {
        text(row);
    }
}

void Immediate::bulletText(const std::string& line) {
    if (canvas_ == nullptr) {
        return;
    }
    const glm::vec2 size(dressing_.indent + measure_(line), dressing_.lineHeight);
    const glm::vec2 corner = place(size);
    const float radius = dressing_.lineHeight * bulletScale * 0.5f;
    canvas_->circle(glm::vec2(corner.x + dressing_.indent * 0.5f, corner.y + dressing_.lineHeight * 0.5f),
        radius, bulletSides, ink(dressing_.text));
    label(line, corner + glm::vec2(dressing_.indent, 0.0f), glm::vec2(size.x, size.y), ink(dressing_.text));
}

bool Immediate::button(const std::string& label) {
    if (canvas_ == nullptr) {
        return false;
    }
    const glm::vec2 size(measure_(label) + dressing_.padding * 2.0f, dressing_.barHeight);
    const glm::vec2 min = place(size);
    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);

    fillBox(canvas_, min, min + size, dressing_.radius, face(reaction.held, reaction.hovered));
    this->label(label, min + glm::vec2(dressing_.padding, 0.0f), size,
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

    fillBox(canvas_, min, min + size, dressing_.radius, face(reaction.held, reaction.hovered));
    this->label(label, min + glm::vec2(dressing_.padding * 0.5f, 0.0f), size,
        ink(reaction.hovered ? dressing_.activeText : dressing_.text));
    return reaction.clicked;
}

bool Immediate::selectable(const std::string& label, bool selected) {
    if (canvas_ == nullptr) {
        return false;
    }
    const glm::vec2 size(right_ - margin_, dressing_.lineHeight);
    const glm::vec2 min = place(size);
    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);

    if (selected) {
        fillBox(canvas_, min, min + size, 0.0f, dressing_.highlight);
    } else if (reaction.hovered) {
        fillBox(canvas_, min, min + size, 0.0f, dressing_.hover);
    }
    this->label(label, min + glm::vec2(dressing_.padding * 0.5f, 0.0f), size,
        ink(selected ? dressing_.activeText : dressing_.text));
    return reaction.clicked;
}

bool Immediate::dragInt(const std::string& label, int* value, int low, int high) {
    if (canvas_ == nullptr || value == nullptr) {
        return false;
    }
    const glm::vec2 size(right_ - margin_, dressing_.barHeight);
    const glm::vec2 min = place(size);
    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);

    bool changed = false;
    if (reaction.held && drag_.x != 0.0f) {
        const int moved = static_cast<int>(std::lround(drag_.x * scrubRate));
        if (moved != 0) {
            const int wanted = std::clamp(*value + moved, low, high);
            changed = wanted != *value;
            *value = wanted;
        }
    }

    fillBox(canvas_, min, min + size, dressing_.radius, face(reaction.held, reaction.hovered));
    std::string shown(label);
    shown.append("  ").append(std::to_string(*value));
    this->label(shown, min + glm::vec2(dressing_.padding, 0.0f), size, ink(dressing_.text));
    return changed;
}

void Immediate::progressBar(float fraction, const std::string& overlay) {
    if (canvas_ == nullptr) {
        return;
    }
    const glm::vec2 size(right_ - margin_, dressing_.lineHeight);
    const glm::vec2 min = place(size);
    const glm::vec2 max = min + size;

    fillBox(canvas_, min, max, dressing_.radius, dressing_.widget);
    const float part = std::clamp(fraction, 0.0f, 1.0f);
    if (part > 0.0f) {
        fillBox(canvas_, min, glm::vec2(min.x + size.x * part, max.y), dressing_.radius, ink(dressing_.fill));
    }
    if (!overlay.empty()) {
        const glm::vec2 corner(min.x + (size.x - measure_(overlay)) * 0.5f, min.y);
        label(overlay, corner, size, ink(dressing_.activeText));
    }
}

void Immediate::separator() {
    if (canvas_ == nullptr) {
        return;
    }
    const glm::vec2 size(right_ - margin_, ruleWidth + dressing_.spacing);
    const glm::vec2 min = place(size);
    const float middle = min.y + dressing_.spacing * 0.5f;
    canvas_->rect(glm::vec2(min.x, middle), glm::vec2(min.x + size.x, middle + ruleWidth), dressing_.rule);
}

void Immediate::sameLine() {
    sameLine_ = true;
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
    tabBar_ = identify(id);
    inTabBar_ = true;
    tabIndex_ = 0;
    tabChanged_ = false;
    tabTaken_ = false;
    // the strip takes a row of its own and the pen goes past it, so what a selected tab
    // holds is drawn under the whole strip rather than beside the next tab
    tabTop_ = penY_;
    tabPen_ = margin_;
    penY_ = tabTop_ + dressing_.barHeight;
    canvas_->rect(glm::vec2(margin_, penY_), glm::vec2(right_, penY_ + ruleWidth), dressing_.rule);
    penY_ += ruleWidth + dressing_.spacing;
    sameLine_ = false;
    return true;
}

bool Immediate::tab(const std::string& label) {
    if (canvas_ == nullptr || !inTabBar_) {
        return false;
    }
    Retained& retained = retain(tabBar_);
    const unsigned int index = tabIndex_++;
    const glm::vec2 size(measure_(label) + dressing_.padding * 2.0f, dressing_.barHeight);
    const glm::vec2 min(tabPen_, tabTop_);
    tabPen_ += size.x + dressing_.spacing;

    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);
    if (reaction.clicked) {
        tabWanted_ = index;
        tabChanged_ = true;
    }
    const bool selected = retained.tab == index;
    tabTaken_ = tabTaken_ || selected;

    fillBox(canvas_, min, min + size, dressing_.radius, face(selected, reaction.hovered));
    this->label(label, min + glm::vec2(dressing_.padding, 0.0f), size,
        selected ? dressing_.activeText : dressing_.text);
    return selected;
}

void Immediate::endTabBar() {
    if (!inTabBar_) {
        return;
    }
    if (tabChanged_) {
        retain(tabBar_).tab = tabWanted_;
    } else if (!tabTaken_ && tabIndex_ > 0) {
        // a strip whose selected tab is no longer there falls back to the first, so a bar
        // is never drawn with nothing chosen
        retain(tabBar_).tab = 0;
    }
    inTabBar_ = false;
}

bool Immediate::table(const std::string& id, unsigned int columns) {
    if (canvas_ == nullptr || columns == 0) {
        return false;
    }
    pushId(id);
    inTable_ = true;
    headers_.clear();
    widths_.assign(columns, 0.0f);
    tableLeft_ = margin_;
    columnIndex_ = 0;
    sameLine_ = false;
    return true;
}

void Immediate::column(const std::string& label, float width) {
    if (!inTable_ || headers_.size() >= widths_.size()) {
        return;
    }
    widths_[headers_.size()] = width;
    headers_.push_back(label);
}

float Immediate::columnStart(unsigned int index) const {
    // a column given no width of its own shares out what the named ones left
    float named = 0.0f;
    unsigned int unnamed = 0;
    for (const float width : widths_) {
        if (width > 0.0f) {
            named += width;
        } else {
            unnamed++;
        }
    }
    const float spare = unnamed == 0 ? 0.0f
        : std::max(0.0f, (right_ - tableLeft_ - named) / static_cast<float>(unnamed));

    float start = tableLeft_;
    for (unsigned int column = 0; column < index && column < widths_.size(); column++) {
        start += widths_[column] > 0.0f ? widths_[column] : spare;
    }
    return start;
}

void Immediate::headerRow() {
    if (canvas_ == nullptr || !inTable_) {
        return;
    }
    const glm::vec2 min(tableLeft_, penY_);
    const glm::vec2 max(right_, penY_ + dressing_.lineHeight);
    fillBox(canvas_, min, max, 0.0f, dressing_.titleBar);
    for (std::size_t index = 0; index < headers_.size(); index++) {
        const glm::vec2 corner(columnStart(static_cast<unsigned int>(index)) + dressing_.padding * 0.5f, min.y);
        label(headers_[index], corner, glm::vec2(0.0f, dressing_.lineHeight), dressing_.dimText);
    }
    penY_ = max.y + dressing_.spacing;
    nextRow();
}

void Immediate::nextRow() {
    if (!inTable_) {
        return;
    }
    columnIndex_ = 0;
    margin_ = columnStart(0);
    sameLine_ = false;
}

void Immediate::nextColumn() {
    if (!inTable_) {
        return;
    }
    columnIndex_++;
    // staying on the row is what sameLine() already does; the column start is where the
    // next widget lands rather than wherever the last one ended
    sameLine_ = true;
    lastRight_ = columnStart(columnIndex_) - dressing_.spacing;
}

void Immediate::endTable() {
    if (!inTable_) {
        return;
    }
    inTable_ = false;
    margin_ = tableLeft_;
    sameLine_ = false;
    popId();
}

};  // namespace v3d::ui
