/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Immediate.h"

#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/immediate/Context.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Painter.h>
#include <api/ui/paint/Text.h>
#include <api/ui/style/Resolver.h>
#include <api/ui/style/Theme.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>

#include <glm/vec2.hpp>

namespace v3d::ui {

namespace {

/**
 * How far a scrub moves the value for each pixel the cursor travels. One unit per pixel is
 * too fast to land on a number by hand and too slow to cross a wide range; a quarter is a
 * compromise between the two.
 **/
const float scrubRate = 0.25F;

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

};  // namespace

Immediate::Input::Input() noexcept :
cursor(0.0F, 0.0F),
down(false),
pressed(false),
released(false),
wheel(0.0F) {
}

Immediate::Immediate(const paint::Measure& measure, const paint::Write& write) :
    context_(std::make_unique<immediate::Context>(measure, write)) {
}

// out of line, so that the header need not complete the context
Immediate::~Immediate() = default;
Immediate::Immediate(Immediate&&) noexcept = default;
Immediate& Immediate::operator=(Immediate&&) noexcept = default;

void Immediate::text(const paint::Measure& measure, const paint::Write& write) {
    context_->measure = measure;
    context_->write = write;
}

paint::Dressing& Immediate::dressing() noexcept {
    return context_->dressing;
}

void Immediate::theme(const boost::shared_ptr<style::Theme>& theme) {
    // the new theme starts from what was there before any theme, so where it is silent the
    // last theme's value does not linger
    context_->overlay.restore(&context_->dressing);
    style::readBase(style::lookup(theme, style::Resolver::tools, std::string_view()), &context_->overlay, &context_->dressing);
}

void Immediate::begin(v3d::render::realtime::canvas::Canvas* canvas, const Input& input) {
    context_->canvas = canvas;
    context_->input = input;
    context_->frame++;
    context_->drag = input.cursor - context_->previousCursor;
    context_->hovering = 0;
    context_->nextWidth = 0.0F;
    context_->ids.clear();
    context_->disabled = 0;
    context_->window.open = false;
    context_->window.clipped = false;
    context_->wheeled = 0;
    context_->tabs.open = false;
    context_->table.open = false;
    context_->table.clipped = false;
    context_->row.sameLine = false;
    context_->row.margin = 0.0F;
    context_->row.penY = 0.0F;
    context_->row.right = canvas == nullptr ? 0.0F : static_cast<float>(canvas->width());
    context_->typingDrawn = false;
    context_->itemMin = glm::vec2(0.0F, 0.0F);
    context_->itemMax = glm::vec2(0.0F, 0.0F);
    context_->itemOwner = 0;
    context_->popup.asked = false;
}

void Immediate::end() {
    // the open combo's list goes over everything, so it is drawn after everything, and takes
    // the cursor from whatever it covers
    context_->drawPopup();

    // a field that was not drawn cannot be typed into, so it does not keep the keys from the
    // app's bindings
    if (context_->typing != 0 && !context_->typingDrawn) {
        context_->focus(0);
    }

    // what the cursor was found on this frame is used by the next one, so a window drawn
    // later takes the cursor from one under it
    context_->hovered = context_->hovering;
    if (context_->input.released || !context_->input.down) {
        context_->active = 0;
    }
    context_->previousCursor = context_->input.cursor;
    context_->canvas = nullptr;

    // what nothing has asked for in a while goes, so that a caller building ids out of
    // changing text costs a bounded amount rather than a growing one
    for (auto entry = context_->states.begin(); entry != context_->states.end();) {
        entry = entry->second.frame + retention < context_->frame ? context_->states.erase(entry) : std::next(entry);
    }
}

void Immediate::block(const glm::vec2& min, const glm::vec2& max) {
    if (paint::inside(min, max, context_->input.cursor)) {
        // an id no widget answers to, so the block takes the cursor and gives it to nothing
        context_->hovering = context_->identify("##block");
    }
}

bool Immediate::capturing() const noexcept {
    // the held half matters as much as the hovered one: a scrubber being dragged keeps the
    // cursor after the drag has left its box, and a click that lands there is still spent
    return context_->hovered != 0 || context_->active != 0;
}

std::size_t Immediate::retained() const noexcept {
    return context_->states.size();
}

void Immediate::pushId(const std::string& id) {
    context_->ids.push_back(context_->identify(id));
}

void Immediate::pushId(int id) {
    pushId(std::to_string(id));
}

void Immediate::popId() {
    if (!context_->ids.empty()) {
        context_->ids.pop_back();
    }
}

void Immediate::nextItemWidth(float width) {
    context_->nextWidth = width;
}

void Immediate::text(const std::string& line) {
    if (context_->canvas == nullptr) {
        return;
    }
    const glm::vec2 size(context_->measure(line), context_->dressing.lineHeight);
    context_->label(line, context_->place(size), size, context_->ink(context_->dressing.text));
}

void Immediate::textDisabled(const std::string& line) {
    if (context_->canvas == nullptr) {
        return;
    }
    const glm::vec2 size(context_->measure(line), context_->dressing.lineHeight);
    context_->label(line, context_->place(size), size, context_->dressing.disabledText);
}

void Immediate::textWrapped(const std::string& line) {
    if (context_->canvas == nullptr) {
        return;
    }
    for (const std::string& row : paint::wrap(line, context_->row.right - context_->row.margin, context_->measure)) {
        text(row);
    }
}

void Immediate::bulletText(const std::string& line) {
    if (context_->canvas == nullptr) {
        return;
    }
    const glm::vec2 size(context_->dressing.indent + context_->measure(line), context_->dressing.lineHeight);
    const glm::vec2 corner = context_->place(size);
    const float radius = context_->dressing.lineHeight * bulletScale * 0.5F;
    context_->canvas->circle(glm::vec2(corner.x + (context_->dressing.indent * 0.5F), corner.y + (context_->dressing.lineHeight * 0.5F)),
        radius, bulletSides, context_->ink(context_->dressing.text));
    context_->label(line, corner + glm::vec2(context_->dressing.indent, 0.0F), glm::vec2(size.x, size.y), context_->ink(context_->dressing.text));
}

bool Immediate::button(const std::string& label) {
    if (context_->canvas == nullptr) {
        return false;
    }
    const glm::vec2 size(context_->measure(label) + (context_->dressing.padding * 2.0F), context_->dressing.barHeight);
    const glm::vec2 min = context_->place(size);
    const Id id = context_->identify(label);
    const immediate::Reaction reaction = context_->interact(id, min, min + size);

    paint::fillBox(context_->canvas, min, min + size, context_->dressing.radius, context_->face(reaction.held, reaction.hovered));
    context_->label(label, min + glm::vec2(context_->dressing.padding, 0.0F), size,
        context_->ink(reaction.hovered ? context_->dressing.activeText : context_->dressing.text));
    return reaction.clicked;
}

bool Immediate::smallButton(const std::string& label) {
    if (context_->canvas == nullptr) {
        return false;
    }
    const glm::vec2 size(context_->measure(label) + context_->dressing.padding, context_->dressing.lineHeight);
    const glm::vec2 min = context_->place(size);
    const Id id = context_->identify(label);
    const immediate::Reaction reaction = context_->interact(id, min, min + size);

    paint::fillBox(context_->canvas, min, min + size, context_->dressing.radius, context_->face(reaction.held, reaction.hovered));
    context_->label(label, min + glm::vec2(context_->dressing.padding * 0.5F, 0.0F), size,
        context_->ink(reaction.hovered ? context_->dressing.activeText : context_->dressing.text));
    return reaction.clicked;
}

bool Immediate::selectable(const std::string& label, bool selected) {
    if (context_->canvas == nullptr) {
        return false;
    }
    const glm::vec2 size(context_->itemWidth(), context_->dressing.lineHeight);
    const glm::vec2 min = context_->place(size);
    const Id id = context_->identify(label);
    const immediate::Reaction reaction = context_->interact(id, min, min + size);

    if (selected) {
        paint::fillBox(context_->canvas, min, min + size, 0.0F, context_->dressing.highlight);
    } else if (reaction.hovered) {
        paint::fillBox(context_->canvas, min, min + size, 0.0F, context_->dressing.hover);
    }
    context_->label(label, min + glm::vec2(context_->dressing.padding * 0.5F, 0.0F), size,
        context_->ink(selected ? context_->dressing.activeText : context_->dressing.text));
    return reaction.clicked;
}

bool Immediate::dragInt(const std::string& label, int* value, int low, int high) {
    if (context_->canvas == nullptr || value == nullptr) {
        return false;
    }
    const glm::vec2 size(context_->itemWidth(), context_->dressing.barHeight);
    const glm::vec2 min = context_->place(size);
    const Id id = context_->identify(label);
    const immediate::Reaction reaction = context_->interact(id, min, min + size);

    bool changed = false;
    if (reaction.held && context_->drag.x != 0.0F) {
        const int moved = static_cast<int>(std::lround(context_->drag.x * scrubRate));
        if (moved != 0) {
            const int wanted = std::clamp(*value + moved, low, high);
            changed = wanted != *value;
            *value = wanted;
        }
    }

    paint::fillBox(context_->canvas, min, min + size, context_->dressing.radius, context_->face(reaction.held, reaction.hovered));
    std::string shown(label);
    shown.append("  ").append(std::to_string(*value));
    context_->label(shown, min + glm::vec2(context_->dressing.padding, 0.0F), size, context_->ink(context_->dressing.text));
    return changed;
}

bool Immediate::checkbox(const std::string& label, bool* value) {
    if (context_->canvas == nullptr || value == nullptr) {
        return false;
    }
    const float side = context_->dressing.lineHeight;
    const glm::vec2 size(side + context_->dressing.padding + context_->measure(label), side);
    const glm::vec2 min = context_->place(size);
    const Id id = context_->identify(label);
    const immediate::Reaction reaction = context_->interact(id, min, min + size);
    if (reaction.clicked) {
        *value = !*value;
    }

    paint::fillBox(context_->canvas, min, min + glm::vec2(side, side), context_->dressing.radius, context_->face(reaction.held, reaction.hovered));
    if (*value) {
        const glm::vec2 inset(side * tickInset, side * tickInset);
        paint::fillBox(context_->canvas, min + inset, min + glm::vec2(side, side) - inset, 0.0F, context_->ink(context_->dressing.mark));
    }
    context_->label(label, min + glm::vec2(side + context_->dressing.padding, 0.0F), size,
        context_->ink(reaction.hovered ? context_->dressing.activeText : context_->dressing.text));
    return reaction.clicked;
}

bool Immediate::sliderFloat(const std::string& label, float* value, float low, float high) {
    if (context_->canvas == nullptr || value == nullptr) {
        return false;
    }
    const glm::vec2 size(context_->itemWidth(), context_->dressing.barHeight);
    const glm::vec2 min = context_->place(size);
    const Id id = context_->identify(label);
    const immediate::Reaction reaction = context_->interact(id, min, min + size);

    // the value is where the cursor is along the track, so a press jumps to it and a drag
    // follows it, and a drag past either end holds the value at that end
    bool changed = false;
    if (reaction.held && size.x > 0.0F) {
        const float along = std::clamp((context_->input.cursor.x - min.x) / size.x, 0.0F, 1.0F);
        const float wanted = low + ((high - low) * along);
        changed = wanted != *value;
        *value = wanted;
    }

    const float span = high - low;
    const float part = span == 0.0F ? 0.0F : std::clamp((*value - low) / span, 0.0F, 1.0F);
    paint::fillBox(context_->canvas, min, min + size, context_->dressing.radius, context_->face(reaction.held, reaction.hovered));
    if (part > 0.0F) {
        paint::fillBox(context_->canvas, min, glm::vec2(min.x + (size.x * part), min.y + size.y), context_->dressing.radius, context_->ink(context_->dressing.fill));
    }
    context_->label(std::format("{}  {:.2f}", label, *value), min + glm::vec2(context_->dressing.padding, 0.0F), size, context_->ink(context_->dressing.text));
    return changed;
}

bool Immediate::itemHovered() const noexcept {
    return paint::inside(context_->itemMin, context_->itemMax, context_->input.cursor) && context_->hovered == context_->itemOwner;
}

void Immediate::progressBar(float fraction, const std::string& overlay) {
    if (context_->canvas == nullptr) {
        return;
    }
    const glm::vec2 size(context_->itemWidth(), context_->dressing.lineHeight);
    const glm::vec2 min = context_->place(size);
    const glm::vec2 max = min + size;

    paint::fillBox(context_->canvas, min, max, context_->dressing.radius, context_->dressing.track);
    const float part = std::clamp(fraction, 0.0F, 1.0F);
    if (part > 0.0F) {
        paint::fillBox(context_->canvas, min, glm::vec2(min.x + (size.x * part), max.y), context_->dressing.radius, context_->ink(context_->dressing.fill));
    }
    if (!overlay.empty()) {
        const glm::vec2 corner(min.x + ((size.x - context_->measure(overlay)) * 0.5F), min.y);
        context_->label(overlay, corner, size, context_->ink(context_->dressing.activeText));
    }
}

void Immediate::separator() {
    if (context_->canvas == nullptr) {
        return;
    }
    const glm::vec2 size(context_->row.right - context_->row.margin, immediate::Context::ruleWidth + context_->dressing.spacing);
    const glm::vec2 min = context_->place(size);
    const float middle = min.y + (context_->dressing.spacing * 0.5F);
    context_->canvas->rect(glm::vec2(min.x, middle), glm::vec2(min.x + size.x, middle + immediate::Context::ruleWidth), context_->dressing.rule);
}

void Immediate::sameLine() {
    context_->row.sameLine = true;
}

void Immediate::beginDisabled() {
    context_->disabled++;
}

void Immediate::endDisabled() {
    if (context_->disabled > 0) {
        context_->disabled--;
    }
}

};  // namespace v3d::ui
