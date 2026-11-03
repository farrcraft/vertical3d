/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Immediate.h>
#include <api/ui/immediate/Context.h>
#include <api/ui/input/LineEdit.h>
#include <api/ui/paint/Painter.h>
#include <api/ui/paint/Type.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#include <glm/vec2.hpp>

namespace v3d::ui {

namespace {

/**
 * @return a number as a field shows it: to three places, with the zeros and the point a whole
 *         number does not need dropped, and nothing that reads as a negative zero
 **/
std::string numeral(float value) {
    std::string shown = std::format("{:.3f}", value);
    const std::size_t point = shown.find('.');
    if (point != std::string::npos) {
        const std::size_t last = shown.find_last_not_of('0');
        shown.erase(last == point ? point : last + 1);
    }
    return shown == "-0" ? "0" : shown;
}

};  // namespace

bool Immediate::press(std::string_view key, bool shifted, bool controlled) {
    if (!typing() || key.empty()) {
        return false;
    }
    if (!controlled && (key == "return" || key == "tab" || key == "escape")) {
        // the field gives the keyboard up when it is next drawn, after it has written what
        // was typed
        context_->letGo = true;
        context_->escaped = key == "escape";
        context_->entered = key == "return";
        return true;
    }
    return input::edit(&context_->edit, key, shifted, controlled, context_->clipboard);
}

bool Immediate::typed(std::string_view utf8) {
    if (!typing() || utf8.empty()) {
        return false;
    }
    // taken even when the limit refused it, so it never reaches the app's bindings
    context_->edit.insert(utf8);
    return true;
}

bool Immediate::composing(std::string_view utf8) {
    if (!typing()) {
        return false;
    }
    context_->edit.compose(utf8);
    return true;
}

bool Immediate::typing() const noexcept {
    return context_->typing != 0 && !context_->letGo;
}

void Immediate::onTyping(const std::function<void(bool)>& changed) {
    context_->onTyping = changed;
}

void Immediate::clipboard(const input::Clipboard& clipboard) {
    context_->clipboard = clipboard;
}

bool Immediate::textField(const std::string& label, std::string* value, std::size_t limit) {
    if (context_->canvas == nullptr || value == nullptr) {
        return false;
    }
    const glm::vec2 size(context_->itemWidth(), context_->dressing.barHeight);
    const glm::vec2 min = context_->place(size);
    const glm::vec2 max = min + size;
    const Id id = context_->identify(label);
    const immediate::Reaction reaction = context_->interact(id, min, max);

    const glm::vec2 low(min.x + context_->dressing.padding, min.y + context_->dressing.borderWidth);
    const glm::vec2 high(max.x - context_->dressing.padding, max.y - context_->dressing.borderWidth);
    const float room = std::max(high.x - low.x, 0.0F);

    if (context_->typing == id && *value != context_->written) {
        // the caller changed the string while it was being edited, and the caller's string
        // is the value, so the edit starts again from it
        context_->edit.text(*value);
        context_->written = *value;
    }
    const bool asked = context_->focusNext && context_->disabled == 0;
    context_->focusNext = false;
    const bool pressed = reaction.hovered && context_->input.pressed;
    if ((pressed || asked) && context_->typing != id) {
        context_->claim(id, *value);
    }
    if (pressed) {
        // the press is measured against the line as it was drawn, slid as far as the caret
        // had pushed it
        const paint::Type type(context_->text, context_->textSize);
        const float slid = std::max(context_->edit.layout(type).caret - room, 0.0F);
        context_->edit.at(context_->input.cursor.x - (low.x - slid), type);
    }

    bool changed = false;
    if (context_->typing == id) {
        context_->typingDrawn = true;
        context_->edit.limit(limit);
        if (context_->edit.text() != *value) {
            *value = std::string(context_->edit.text());
            context_->written = *value;
            changed = true;
        }
        // a press anywhere else, a disabled field, and a return, tab or escape all give the
        // keyboard up. What was typed has been written by then
        if (context_->letGo || context_->disabled > 0 || (context_->input.pressed && !reaction.hovered)) {
            context_->itemEntered = context_->letGo && context_->entered;
            context_->focus(0);
            context_->itemFinished = true;
        }
    }

    const bool focused = context_->typing == id;
    context_->itemTyping = focused;
    paint::plateBox(context_->canvas, min, max, context_->dressing.radius, context_->dressing.borderWidth,
        {.inside = context_->eased(id, context_->face(false, reaction.hovered && !focused)), .outline = focused ? context_->dressing.focus : context_->dressing.border});
    if (high.x <= low.x) {
        return changed;
    }
    context_->canvas->clip(low, high);
    if (!focused && value->empty()) {
        context_->label(label, low, glm::vec2(room, size.y - (context_->dressing.borderWidth * 2.0F)), context_->dressing.placeholder);
        context_->canvas->unclip();
        return changed;
    }

    if (focused) {
        context_->drawEdit(low, high);
    } else {
        context_->label(*value, low, glm::vec2(room, high.y - low.y), context_->ink(context_->dressing.text));
    }
    context_->canvas->unclip();
    return changed;
}

bool Immediate::dragFloat(const std::string& label, float* value, float speed) {
    return dragFloat(label, value, speed, NumberLook{});
}

bool Immediate::dragFloat(const std::string& label, float* value, float speed, const NumberLook& look) {
    if (context_->canvas == nullptr || value == nullptr) {
        return false;
    }
    const glm::vec2 size(context_->itemWidth(), context_->dressing.barHeight);
    const glm::vec2 min = context_->place(size);
    const glm::vec2 max = min + size;
    const Id id = context_->identify(label);
    const immediate::Reaction reaction = context_->interact(id, min, max);

    bool changed = false;
    if (context_->typing == id) {
        changed = context_->typeNumber(reaction, value);
    } else {
        changed = context_->dragNumber(id, reaction, value, speed);
    }
    context_->drawNumber(
        {.id = id, .label = label, .value = *value, .min = min, .max = max, .labelColour = look.labelColour, .unit = look.unit}, reaction);
    return changed;
}

bool immediate::Context::typeNumber(const Reaction& reaction, float* value) {
    typingDrawn = true;
    // a typed value is taken as the field lets go, so a half typed number never reaches the
    // caller
    if (!letGo && disabled == 0 && (!input.pressed || reaction.hovered)) {
        return false;
    }
    const std::optional<float> typed = escaped ? std::nullopt : number(edit.text());
    focus(0);
    itemFinished = true;
    if (!typed || *typed == *value) {
        return false;
    }
    *value = *typed;
    return true;
}

bool immediate::Context::dragNumber(Id id, const Reaction& reaction, float* value, float speed) {
    Retained& retained = retain(id);
    if (!reaction.held) {
        retained.dragging = false;
        return false;
    }
    // a press becomes a drag once it has travelled, and the pixels spent deciding are not
    // applied. One that never travelled is a click, which types into the field
    if (!retained.dragging && std::abs(input.cursor.x - pressAt.x) > dragThreshold) {
        retained.dragging = true;
    }
    bool changed = false;
    const float wanted = *value + (drag.x * speed);
    if (retained.dragging && std::isfinite(wanted) && wanted != *value) {
        *value = wanted;
        changed = true;
    }
    if (input.released) {
        if (retained.dragging) {
            itemFinished = true;
        } else if (reaction.clicked) {
            focus(id);
            typingDrawn = true;
            edit.text(std::format("{}", *value));
            edit.selectAll();
        }
        retained.dragging = false;
    }
    return changed;
}

void immediate::Context::drawNumber(const Number& field, const Reaction& reaction) {
    const bool focused = typing == field.id;
    const bool dragging = reaction.held && retain(field.id).dragging;
    if (focused) {
        paint::plateBox(canvas, field.min, field.max, dressing.radius, dressing.borderWidth,
            {.inside = dressing.widget, .outline = dressing.focus});
    } else {
        paint::fillBox(canvas, field.min, field.max, dressing.radius, eased(field.id, face(dragging, reaction.hovered)));
    }
    const glm::vec2 low(field.min.x + dressing.padding, field.min.y + dressing.borderWidth);
    const glm::vec2 high(field.max.x - dressing.padding, field.max.y - dressing.borderWidth);
    if (high.x <= low.x) {
        return;
    }
    canvas->clip(low, high);
    if (focused) {
        drawEdit(low, high);
    } else {
        const glm::vec2 room(high.x - low.x, high.y - low.y);
        label(field.label, low, room, ink(field.labelColour.value_or(dressing.secondaryText)));
        const std::string shown = numeral(field.value) + field.unit;
        // the value sits at the right, and never starts before the label has ended
        const float left = std::max(high.x - measure(shown), low.x + measure(field.label) + dressing.spacing);
        label(shown, glm::vec2(left, low.y), room, ink(dressing.text));
    }
    canvas->unclip();
}

void immediate::Context::drawEdit(const glm::vec2& low, const glm::vec2& high) {
    // the line slides left when the caret would be past the far edge, so a full field can
    // still be typed into
    const float room = std::max(high.x - low.x, 0.0F);
    const input::LineEdit::Layout laid = edit.layout(paint::Type(text, textSize));
    const float pen = low.x - std::max(laid.caret - room, 0.0F);
    // the highlight goes behind the line, which is drawn whole in one colour
    for (const glm::vec2& run : laid.selected) {
        paint::fillBox(canvas, glm::vec2(pen + run.x, low.y), glm::vec2(pen + run.y, high.y), 0.0F, dressing.highlight);
    }
    const glm::vec4 inked = ink(dressing.text);
    label(laid.text, glm::vec2(pen, low.y), glm::vec2(room, high.y - low.y), inked);
    const float stem = std::max(dressing.borderWidth, 1.0F);
    // a composition is underlined where it stands in the line, below its baseline
    const float baseline = low.y + (((high.y - low.y) + (dressing.lineHeight * 0.5F)) * 0.5F);
    for (const glm::vec2& run : laid.composed) {
        canvas->rect(glm::vec2(pen + run.x, baseline + stem), glm::vec2(pen + run.y, baseline + (stem * 2.0F)), inked);
    }
    canvas->rect(glm::vec2(pen + laid.caret, low.y), glm::vec2(pen + laid.caret + stem, high.y), dressing.caret);
}

std::optional<float> immediate::Context::number(std::string_view text) {
    while (!text.empty() && text.front() == ' ') {
        text.remove_prefix(1);
    }
    while (!text.empty() && text.back() == ' ') {
        text.remove_suffix(1);
    }
    if (!text.empty() && text.front() == '+') {
        text.remove_prefix(1);
    }
    float parsed = 0.0F;
    const std::from_chars_result read = std::from_chars(text.data(), text.data() + text.size(), parsed);
    if (text.empty() || read.ec != std::errc() || read.ptr != text.data() + text.size() || !std::isfinite(parsed)) {
        return std::nullopt;
    }
    return parsed;
}

};  // namespace v3d::ui
