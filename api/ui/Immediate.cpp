/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Immediate.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#include "Painter.h"

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
released(false) {
}

Immediate::Reaction::Reaction() noexcept :
hovered(false),
held(false),
clicked(false) {
}

Immediate::Retained::Retained() noexcept :
tab(0),
collapsed(false) {
}

Immediate::Style::Style() noexcept :
lineHeight(18.0f),
padding(6.0f),
spacing(4.0f),
barHeight(22.0f),
borderWidth(1.0f),
radius(3.0f),
indent(14.0f),
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

Immediate::Style& Immediate::style() noexcept {
    return style_;
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

    readColour(chrome, "panel", &style_.panel);
    readColour(chrome, "border", &style_.border);
    readColour(chrome, "title-bar", &style_.titleBar);
    readColour(chrome, "text", &style_.text);
    readColour(chrome, "active-text", &style_.activeText);
    readColour(chrome, "dim-text", &style_.dimText);
    readColour(chrome, "widget", &style_.widget);
    readColour(chrome, "highlight", &style_.highlight);
    readColour(chrome, "hover", &style_.hover);
    readColour(chrome, "fill", &style_.fill);
    readColour(chrome, "rule", &style_.rule);

    readMetric(chrome, "line-height", &style_.lineHeight);
    readMetric(chrome, "padding", &style_.padding);
    readMetric(chrome, "spacing", &style_.spacing);
    readMetric(chrome, "bar-height", &style_.barHeight);
    readMetric(chrome, "border-width", &style_.borderWidth);
    readMetric(chrome, "radius", &style_.radius);
}

void Immediate::begin(v3d::render::realtime::Canvas* canvas, const Input& input) {
    canvas_ = canvas;
    input_ = input;
    drag_ = input.cursor - previousCursor_;
    hovering_ = 0;
    ids_.clear();
    disabled_ = 0;
    inWindow_ = false;
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
        corner = glm::vec2(lastRight_ + style_.spacing, rowTop_);
        sameLine_ = false;
    } else {
        rowTop_ = penY_;
        rowHeight_ = 0.0f;
        corner = glm::vec2(margin_, rowTop_);
    }
    lastRight_ = corner.x + size.x;
    rowHeight_ = std::max(rowHeight_, size.y);
    penY_ = rowTop_ + rowHeight_ + style_.spacing;
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
    return disabled_ > 0 ? style_.dimText : colour;
}

glm::vec4 Immediate::face(bool lit, bool hovered) const {
    if (disabled_ > 0) {
        return style_.widget;
    }
    if (lit) {
        return style_.highlight;
    }
    return hovered ? style_.hover : style_.widget;
}

void Immediate::label(const std::string& line, const glm::vec2& min, const glm::vec2& size,
    const glm::vec4& colour) const {
    // the baseline sits three quarters of the way down a row, which is where a font with
    // ordinary descenders looks centred
    const glm::vec2 pen(min.x, min.y + (size.y + style_.lineHeight * 0.5f) * 0.5f);
    write_(line, pen, colour);
}

bool Immediate::window(const std::string& title, const glm::vec2& position, const glm::vec2& size,
    float alpha) {
    if (canvas_ == nullptr) {
        return false;
    }
    const Id id = identify(title);
    Retained& retained = state_[id];

    const glm::vec2 min = position;
    const glm::vec2 barMax(position.x + size.x, position.y + style_.barHeight);
    const glm::vec2 max = retained.collapsed ? barMax : position + size;

    glm::vec4 background = style_.panel;
    background.a *= std::clamp(alpha, 0.0f, 1.0f);
    plateBox(canvas_, min, max, style_.radius, style_.borderWidth, background, style_.border);
    fillBox(canvas_, min + glm::vec2(style_.borderWidth, style_.borderWidth),
        glm::vec2(barMax.x - style_.borderWidth, barMax.y), style_.radius, style_.titleBar);
    label(title, min + glm::vec2(style_.padding, 0.0f), glm::vec2(size.x, style_.barHeight), style_.text);

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
    margin_ = min.x + style_.padding;
    right_ = max.x - style_.padding;
    penY_ = barMax.y + style_.spacing;
    sameLine_ = false;
    return !retained.collapsed;
}

void Immediate::endWindow() {
    if (!inWindow_) {
        return;
    }
    margin_ = windowMargin_;
    right_ = windowRight_;
    inWindow_ = false;
    sameLine_ = false;
}

void Immediate::text(const std::string& line) {
    if (canvas_ == nullptr) {
        return;
    }
    const glm::vec2 size(measure_(line), style_.lineHeight);
    label(line, place(size), size, ink(style_.text));
}

void Immediate::textDisabled(const std::string& line) {
    if (canvas_ == nullptr) {
        return;
    }
    const glm::vec2 size(measure_(line), style_.lineHeight);
    label(line, place(size), size, style_.dimText);
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
    const glm::vec2 size(style_.indent + measure_(line), style_.lineHeight);
    const glm::vec2 corner = place(size);
    const float radius = style_.lineHeight * bulletScale * 0.5f;
    canvas_->circle(glm::vec2(corner.x + style_.indent * 0.5f, corner.y + style_.lineHeight * 0.5f),
        radius, bulletSides, ink(style_.text));
    label(line, corner + glm::vec2(style_.indent, 0.0f), glm::vec2(size.x, size.y), ink(style_.text));
}

bool Immediate::button(const std::string& label) {
    if (canvas_ == nullptr) {
        return false;
    }
    const glm::vec2 size(measure_(label) + style_.padding * 2.0f, style_.barHeight);
    const glm::vec2 min = place(size);
    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);

    fillBox(canvas_, min, min + size, style_.radius, face(reaction.held, reaction.hovered));
    this->label(label, min + glm::vec2(style_.padding, 0.0f), size,
        ink(reaction.hovered ? style_.activeText : style_.text));
    return reaction.clicked;
}

bool Immediate::smallButton(const std::string& label) {
    if (canvas_ == nullptr) {
        return false;
    }
    const glm::vec2 size(measure_(label) + style_.padding, style_.lineHeight);
    const glm::vec2 min = place(size);
    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);

    fillBox(canvas_, min, min + size, style_.radius, face(reaction.held, reaction.hovered));
    this->label(label, min + glm::vec2(style_.padding * 0.5f, 0.0f), size,
        ink(reaction.hovered ? style_.activeText : style_.text));
    return reaction.clicked;
}

bool Immediate::selectable(const std::string& label, bool selected) {
    if (canvas_ == nullptr) {
        return false;
    }
    const glm::vec2 size(right_ - margin_, style_.lineHeight);
    const glm::vec2 min = place(size);
    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);

    if (selected) {
        fillBox(canvas_, min, min + size, 0.0f, style_.highlight);
    } else if (reaction.hovered) {
        fillBox(canvas_, min, min + size, 0.0f, style_.hover);
    }
    this->label(label, min + glm::vec2(style_.padding * 0.5f, 0.0f), size,
        ink(selected ? style_.activeText : style_.text));
    return reaction.clicked;
}

bool Immediate::dragInt(const std::string& label, int* value, int low, int high) {
    if (canvas_ == nullptr || value == nullptr) {
        return false;
    }
    const glm::vec2 size(right_ - margin_, style_.barHeight);
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

    fillBox(canvas_, min, min + size, style_.radius, face(reaction.held, reaction.hovered));
    std::string shown(label);
    shown.append("  ").append(std::to_string(*value));
    this->label(shown, min + glm::vec2(style_.padding, 0.0f), size, ink(style_.text));
    return changed;
}

void Immediate::progressBar(float fraction, const std::string& overlay) {
    if (canvas_ == nullptr) {
        return;
    }
    const glm::vec2 size(right_ - margin_, style_.lineHeight);
    const glm::vec2 min = place(size);
    const glm::vec2 max = min + size;

    fillBox(canvas_, min, max, style_.radius, style_.widget);
    const float part = std::clamp(fraction, 0.0f, 1.0f);
    if (part > 0.0f) {
        fillBox(canvas_, min, glm::vec2(min.x + size.x * part, max.y), style_.radius, ink(style_.fill));
    }
    if (!overlay.empty()) {
        const glm::vec2 corner(min.x + (size.x - measure_(overlay)) * 0.5f, min.y);
        label(overlay, corner, size, ink(style_.activeText));
    }
}

void Immediate::separator() {
    if (canvas_ == nullptr) {
        return;
    }
    const glm::vec2 size(right_ - margin_, ruleWidth + style_.spacing);
    const glm::vec2 min = place(size);
    const float middle = min.y + style_.spacing * 0.5f;
    canvas_->rect(glm::vec2(min.x, middle), glm::vec2(min.x + size.x, middle + ruleWidth), style_.rule);
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
    penY_ = tabTop_ + style_.barHeight;
    canvas_->rect(glm::vec2(margin_, penY_), glm::vec2(right_, penY_ + ruleWidth), style_.rule);
    penY_ += ruleWidth + style_.spacing;
    sameLine_ = false;
    return true;
}

bool Immediate::tab(const std::string& label) {
    if (canvas_ == nullptr || !inTabBar_) {
        return false;
    }
    Retained& retained = state_[tabBar_];
    const unsigned int index = tabIndex_++;
    const glm::vec2 size(measure_(label) + style_.padding * 2.0f, style_.barHeight);
    const glm::vec2 min(tabPen_, tabTop_);
    tabPen_ += size.x + style_.spacing;

    const Id id = identify(label);
    const Reaction reaction = interact(id, min, min + size);
    if (reaction.clicked) {
        tabWanted_ = index;
        tabChanged_ = true;
    }
    const bool selected = retained.tab == index;
    tabTaken_ = tabTaken_ || selected;

    fillBox(canvas_, min, min + size, style_.radius, face(selected, reaction.hovered));
    this->label(label, min + glm::vec2(style_.padding, 0.0f), size,
        selected ? style_.activeText : style_.text);
    return selected;
}

void Immediate::endTabBar() {
    if (!inTabBar_) {
        return;
    }
    if (tabChanged_) {
        state_[tabBar_].tab = tabWanted_;
    } else if (!tabTaken_ && tabIndex_ > 0) {
        // a strip whose selected tab is no longer there falls back to the first, so a bar
        // is never drawn with nothing chosen
        state_[tabBar_].tab = 0;
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
    const glm::vec2 max(right_, penY_ + style_.lineHeight);
    fillBox(canvas_, min, max, 0.0f, style_.titleBar);
    for (std::size_t index = 0; index < headers_.size(); index++) {
        const glm::vec2 corner(columnStart(static_cast<unsigned int>(index)) + style_.padding * 0.5f, min.y);
        label(headers_[index], corner, glm::vec2(0.0f, style_.lineHeight), style_.dimText);
    }
    penY_ = max.y + style_.spacing;
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
    lastRight_ = columnStart(columnIndex_) - style_.spacing;
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
