/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Immediate.h>
#include <api/ui/input/LineEdit.h>
#include <api/ui/paint/Painter.h>

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

#include "Context.h"

namespace v3d::ui {

bool Immediate::press(std::string_view key, bool shifted, bool controlled) {
    if (!typing() || key.empty()) {
        return false;
    }
    if (!controlled && (key == "return" || key == "tab" || key == "escape")) {
        // the field gives the keyboard up when it is next drawn, after it has written what
        // was typed
        context_->letGo = true;
        context_->escaped = key == "escape";
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
    if (reaction.hovered && context_->input.pressed) {
        if (context_->typing != id) {
            context_->focus(id);
            context_->edit.text(*value);
            context_->written = *value;
        }
        // the press is measured against the line as it was drawn, slid as far as the caret
        // had pushed it
        const float caret = context_->measure(context_->edit.text().substr(0, context_->edit.caret()));
        const float slid = std::max(caret - room, 0.0F);
        context_->edit.caret(context_->edit.at(context_->input.cursor.x - (low.x - slid), context_->measure));
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
            context_->focus(0);
            context_->itemFinished = true;
        }
    }

    const bool focused = context_->typing == id;
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
    context_->drawNumber({.id = id, .label = label, .value = *value, .min = min, .max = max}, reaction);
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
        label(field.label, low, room, ink(dressing.secondaryText));
        const std::string shown = std::format("{:.3f}", field.value);
        label(shown, glm::vec2(high.x - measure(shown), low.y), room, ink(dressing.text));
    }
    canvas->unclip();
}

void immediate::Context::drawEdit(const glm::vec2& low, const glm::vec2& high) {
    // the line slides left when the caret would be past the far edge, so a full field can
    // still be typed into
    const float room = std::max(high.x - low.x, 0.0F);
    const std::string_view line = edit.text();
    const float caret = measure(line.substr(0, edit.caret()));
    const float pen = low.x - std::max(caret - room, 0.0F);
    if (edit.selected()) {
        // the highlight goes behind the line, which is drawn whole in one colour, so the
        // runs either side of it cannot measure differently from the prefix that places the
        // caret
        const float from = measure(line.substr(0, std::min(edit.anchor(), edit.caret())));
        const float to = measure(line.substr(0, std::max(edit.anchor(), edit.caret())));
        paint::fillBox(canvas, glm::vec2(pen + from, low.y), glm::vec2(pen + to, high.y), 0.0F, dressing.highlight);
    }
    label(std::string(line), glm::vec2(pen, low.y), glm::vec2(room, high.y - low.y), ink(dressing.text));
    const float stem = std::max(dressing.borderWidth, 1.0F);
    canvas->rect(glm::vec2(pen + caret, low.y), glm::vec2(pen + caret + stem, high.y), dressing.caret);
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
