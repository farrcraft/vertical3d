/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Immediate.h"

#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/Checked.h>
#include <api/ui/Image.h>
#include <api/ui/Transition.h>
#include <api/ui/immediate/Context.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Painter.h>
#include <api/ui/paint/Palette.h>
#include <api/ui/paint/TextRenderer.h>
#include <api/ui/style/Resolver.h>
#include <api/ui/style/Theme.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui {

Immediate::Input::Input() noexcept :
cursor(0.0F, 0.0F),
down(false),
pressed(false),
released(false),
wheel(0.0F) {
}

Immediate::Immediate(const boost::shared_ptr<paint::TextRenderer>& text, float size) :
    context_(std::make_unique<immediate::Context>(text, size)) {
}

// out of line, so that the header need not complete the context
Immediate::~Immediate() = default;
Immediate::Immediate(Immediate&&) noexcept = default;
Immediate& Immediate::operator=(Immediate&&) noexcept = default;

void Immediate::text(const boost::shared_ptr<paint::TextRenderer>& text, float size) {
    context_->text = text;
    context_->textSize = size;
}

void Immediate::clock(const Clock& now) noexcept {
    context_->clock = now;
    context_->timed = true;
}

void Immediate::scale(float factor) {
    // the theme's sizes were read at the last scale, so they are put back before the new one
    context_->overlay.restore(&context_->dressing);
    context_->scale = factor;
    context_->dressing.resize(paint::Dressing::tools().scaled(factor));
}

paint::Dressing& Immediate::dressing() noexcept {
    return context_->dressing;
}

void Immediate::theme(const boost::shared_ptr<style::Theme>& theme) {
    // the new theme starts from what was there before any theme, so where it is silent the
    // last theme's value does not linger
    context_->overlay.restore(&context_->dressing);
    style::readBase(style::lookup(theme, style::Resolver::tools, std::string_view()), paint::Palette::Use::Tools,
        &context_->overlay, &context_->dressing, context_->scale);
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
    context_->drawTip();

    // a field that was not drawn cannot be typed into, so it does not keep the keys from the
    // app's bindings
    if (context_->typing != 0 && !context_->typingDrawn) {
        context_->focus(0);
    }
    context_->focusNext = false;

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

float Immediate::rowWidth() const noexcept {
    return std::max(context_->row.right - context_->row.margin, 0.0F);
}

float Immediate::roomBelow() const noexcept {
    if (!context_->window.open) {
        return 0.0F;
    }
    // a row is followed by the gap between rows, and a window whose content ends past its body
    // scrolls, so the gap is kept inside the body too
    return std::max(context_->window.bodyMax.y - context_->dressing.spacing - context_->row.penY, 0.0F);
}

void Immediate::text(const std::string& line) {
    if (context_->canvas == nullptr) {
        return;
    }
    const glm::vec2 size(context_->measure(line), context_->dressing.lineHeight);
    context_->label(line, context_->place(size), size, context_->ink(context_->dressing.text));
}

void Immediate::text(const std::string& line, const glm::vec4& colour) {
    if (context_->canvas == nullptr) {
        return;
    }
    const glm::vec2 size(context_->measure(line), context_->dressing.lineHeight);
    context_->label(line, context_->place(size), size, context_->ink(colour));
}

void Immediate::image(const Image& picture, const glm::vec2& size, const glm::vec4& tint) {
    if (context_->canvas == nullptr) {
        return;
    }
    // the item is at least a line tall, so it keeps the row's height beside text
    const glm::vec2 room(size.x, std::max(size.y, context_->dressing.lineHeight));
    const glm::vec2 corner = context_->place(room) + glm::vec2(0.0F, (room.y - size.y) * 0.5F);
    if (picture.valid()) {
        context_->canvas->rect(corner, corner + size, picture.uv0, picture.uv1, context_->ink(tint), picture.texture);
    }
}

void Immediate::textDisabled(const std::string& line) {
    if (context_->canvas == nullptr) {
        return;
    }
    const glm::vec2 size(context_->measure(line), context_->dressing.lineHeight);
    context_->label(line, context_->place(size), size, context_->dressing.disabledText);
}

void Immediate::textWrapped(const std::string& line) {
    if (context_->canvas == nullptr || !context_->text) {
        return;
    }
    for (const std::string& row : context_->text->rows(line, context_->row.right - context_->row.margin, context_->text->style(context_->textSize))) {
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

    paint::fillBox(context_->canvas, min, min + size, context_->dressing.radius, context_->eased(id, context_->face(reaction.held, reaction.hovered)));
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

    paint::fillBox(context_->canvas, min, min + size, context_->dressing.radius, context_->eased(id, context_->face(reaction.held, reaction.hovered)));
    context_->label(label, min + glm::vec2(context_->dressing.padding * 0.5F, 0.0F), size,
        context_->ink(reaction.hovered ? context_->dressing.activeText : context_->dressing.text));
    return reaction.clicked;
}

bool Immediate::iconButton(const std::string& id, const IconLook& look) {
    if (context_->canvas == nullptr) {
        return false;
    }
    const float bar = context_->dressing.barHeight;
    const glm::vec2 min = context_->place(glm::vec2(bar, bar));
    const glm::vec2 max = min + glm::vec2(bar, bar);
    const Id named = context_->identify(id);
    const immediate::Reaction reaction = context_->interact(named, min, max);

    paint::fillBox(context_->canvas, min, max, context_->dressing.radius, context_->eased(named, context_->face(look.on || reaction.held, reaction.hovered)));
    if (look.icon.valid()) {
        const float side = std::min(context_->dressing.iconSize, bar);
        const glm::vec2 corner = min + glm::vec2((bar - side) * 0.5F);
        const glm::vec4 ink = context_->ink(look.on || reaction.hovered ? context_->dressing.activeText : context_->dressing.text);
        context_->canvas->rect(corner, corner + glm::vec2(side), look.icon.uv0, look.icon.uv1, ink, look.icon.texture);
    }
    if (reaction.hovered && (!look.tooltip.empty() || !look.keys.empty())) {
        context_->askTip(named, look.tooltip, look.keys, min, max);
    }
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

bool Immediate::treeRow(const std::string& label, const TreeRow& row) {
    if (context_->canvas == nullptr) {
        return false;
    }
    const float line = context_->dressing.lineHeight;
    const glm::vec2 size(context_->itemWidth(), line);
    const glm::vec2 min = context_->place(size);
    const glm::vec2 max = min + size;
    const Id id = context_->identify(label);
    const immediate::Reaction reaction = context_->interact(id, min, max);

    // the mark is offered after the row, so the cursor on it is the mark's and not the row's
    const glm::vec2 markMin(min.x + (line * static_cast<float>(row.depth)), min.y);
    const glm::vec2 markMax = markMin + glm::vec2(line, line);
    immediate::Reaction fold;
    if (row.open != nullptr) {
        fold = context_->interact(context_->identify(label + " fold"), markMin, markMax);
        if (fold.clicked) {
            *row.open = !*row.open;
        }
    }
    // the end icon is offered after the row too, so a click on it is not a click on the row
    const float side = std::min(context_->dressing.iconSize, line);
    const glm::vec2 endMin(max.x - line, min.y);
    immediate::Reaction ending;
    if (row.end.valid()) {
        ending = context_->interact(context_->identify(label + " end"), endMin, max);
    }
    if (row.endClicked != nullptr) {
        *row.endClicked = ending.clicked;
    }

    if (row.selected) {
        paint::fillBox(context_->canvas, min, max, 0.0F, context_->dressing.selected);
    } else if (reaction.hovered || fold.hovered || ending.hovered) {
        paint::fillBox(context_->canvas, min, max, 0.0F, context_->dressing.hover);
    }
    glm::vec4 ink = context_->ink(row.selected ? context_->dressing.activeText : context_->dressing.text);
    glm::vec4 tint = row.iconTint;
    if (row.dimmed) {
        ink = context_->ink(context_->dressing.disabledText);
        tint.a *= 0.5F;
    }
    if (row.open != nullptr) {
        foldMark((markMin + markMax) * 0.5F, *row.open, ink);
    }
    // the icon sits between the mark's place and the label, centred down the row
    float left = markMax.x;
    if (row.icon.valid()) {
        const glm::vec2 corner(left, min.y + ((line - side) * 0.5F));
        context_->canvas->rect(corner, corner + glm::vec2(side, side), row.icon.uv0, row.icon.uv1, context_->ink(tint), row.icon.texture);
        left += side + (context_->dressing.padding * 0.5F);
    }
    float right = max.x;
    if (row.end.valid()) {
        const glm::vec2 corner(endMin.x + ((line - side) * 0.5F), min.y + ((line - side) * 0.5F));
        context_->canvas->rect(corner, corner + glm::vec2(side, side), row.end.uv0, row.end.uv1, context_->ink(context_->dressing.secondaryText),
            row.end.texture);
        right = endMin.x;
    }
    // a long label is cut short of the end icon rather than drawn under it
    if (row.end.valid()) {
        context_->canvas->clip(glm::vec2(left, min.y), glm::vec2(std::max(left, right), max.y));
    }
    context_->label(label, glm::vec2(left, min.y), glm::vec2(std::max(right - left, 0.0F), line), ink);
    if (row.end.valid()) {
        context_->canvas->unclip();
    }
    return reaction.clicked;
}

bool Immediate::heading(const std::string& label, bool* open) {
    if (context_->canvas == nullptr) {
        return open == nullptr || *open;
    }
    const float line = context_->dressing.lineHeight;
    const glm::vec2 size(context_->itemWidth(), line);
    const glm::vec2 min = context_->place(size);
    const glm::vec2 max = min + size;
    float left = min.x;
    if (open != nullptr) {
        const Id id = context_->identify(label);
        const immediate::Reaction reaction = context_->interact(id, min, max);
        if (reaction.clicked) {
            *open = !*open;
        }
        if (reaction.hovered) {
            paint::fillBox(context_->canvas, min, max, 0.0F, context_->dressing.hover);
        }
        foldMark(min + glm::vec2(line * 0.5F), *open, context_->ink(context_->dressing.secondaryText));
        left += line;
    }
    context_->title(label, glm::vec2(left, min.y), glm::vec2(max.x - left, line), context_->ink(context_->dressing.text));
    return open == nullptr || *open;
}

void Immediate::foldMark(const glm::vec2& centre, bool open, const glm::vec4& ink) {
    const float line = context_->dressing.lineHeight;
    const float reach = line * foldScale;
    const float width = std::max(line * foldWidth, 1.0F);
    const glm::vec2 tip = open ? glm::vec2(0.0F, reach * 0.5F) : glm::vec2(reach * 0.5F, 0.0F);
    const glm::vec2 side = open ? glm::vec2(reach, 0.0F) : glm::vec2(0.0F, reach);
    const glm::vec2 back = open ? glm::vec2(0.0F, -reach * 0.5F) : glm::vec2(-reach * 0.5F, 0.0F);
    context_->canvas->segment({.from = centre + back - side, .to = centre + tip, .width = width}, ink);
    context_->canvas->segment({.from = centre + tip, .to = centre + back + side, .width = width}, ink);
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
        // the cursor comes from the caller, so a step that is not finite or not a plausible
        // distance is ignored
        const std::optional<int32_t> moved = v3d::type::toInteger(std::round(context_->drag.x * scrubRate),
            {.minimum = -scrubLimit, .maximum = scrubLimit});
        if (moved.has_value() && *moved != 0) {
            const int wanted = static_cast<int>(std::clamp(static_cast<int64_t>(*value) + *moved, static_cast<int64_t>(low),
                static_cast<int64_t>(high)));
            changed = wanted != *value;
            *value = wanted;
        }
    }

    paint::fillBox(context_->canvas, min, min + size, context_->dressing.radius, context_->eased(id, context_->face(reaction.held, reaction.hovered)));
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

    paint::fillBox(context_->canvas, min, min + glm::vec2(side, side), context_->dressing.radius, context_->eased(id, context_->face(reaction.held, reaction.hovered)));
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
    paint::fillBox(context_->canvas, min, min + size, context_->dressing.radius, context_->eased(id, context_->face(reaction.held, reaction.hovered)));
    if (part > 0.0F) {
        paint::fillBox(context_->canvas, min, glm::vec2(min.x + (size.x * part), min.y + size.y), context_->dressing.radius, context_->ink(context_->dressing.fill));
    }
    context_->label(std::format("{}  {:.2f}", label, *value), min + glm::vec2(context_->dressing.padding, 0.0F), size, context_->ink(context_->dressing.text));
    return changed;
}

bool Immediate::itemHovered() const noexcept {
    return paint::inside(context_->itemMin, context_->itemMax, context_->input.cursor) && context_->hovered == context_->itemOwner;
}

bool Immediate::itemFinished() const noexcept {
    return context_->itemFinished;
}

bool Immediate::itemEntered() const noexcept {
    return context_->itemEntered;
}

bool Immediate::itemTyping() const noexcept {
    return context_->itemTyping;
}

void Immediate::focusNext() noexcept {
    context_->focusNext = true;
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

Immediate::Surface Immediate::surface(const std::string& id, float height) {
    Surface made;
    if (context_->canvas == nullptr || !std::isfinite(height) || height <= 0.0F) {
        return made;
    }
    const glm::vec2 size(context_->itemWidth(), height);
    made.min = context_->place(size);
    made.max = made.min + size;
    const Id identity = context_->identify(id);
    const immediate::Reaction reaction = context_->interact(identity, made.min, made.max);
    made.cursor = context_->input.cursor;
    made.hovered = reaction.hovered;
    made.held = reaction.held;
    made.pressed = reaction.held && reaction.hovered && context_->input.pressed;
    made.released = reaction.held && context_->input.released;
    made.pressAt = reaction.held ? context_->pressAt : made.cursor;
    made.wheel = made.hovered ? context_->input.wheel : 0.0F;
    return made;
}

void Immediate::fill(const glm::vec2& min, const glm::vec2& max, const glm::vec4& colour) {
    if (context_->canvas != nullptr) {
        context_->canvas->rect(min, max, colour);
    }
}

void Immediate::line(const glm::vec2& from, const glm::vec2& to, float width, const glm::vec4& colour) {
    if (context_->canvas != nullptr) {
        context_->canvas->segment({ .from = from, .to = to, .width = width }, colour);
    }
}

void Immediate::diamond(const glm::vec2& centre, float radius, const glm::vec4& colour) {
    if (context_->canvas != nullptr && std::isfinite(radius) && radius > 0.0F) {
        // four sides from the right-hand point make a square stood on its corner
        context_->canvas->circle(centre, radius, 4, colour);
    }
}

void Immediate::textAt(const std::string& line, const glm::vec2& at, const glm::vec4& colour) {
    if (context_->canvas != nullptr) {
        context_->label(line, at, glm::vec2(context_->measure(line), context_->dressing.lineHeight), colour);
    }
}

float Immediate::measure(const std::string& line) const {
    return context_->measure(line);
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
