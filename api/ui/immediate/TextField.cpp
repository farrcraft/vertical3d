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

namespace {

/**
 * How far a press on a number has to travel across it before it drags the value instead of
 * typing into it, as a window's title bar decides between a move and a fold.
 **/
const float dragThreshold = 3.0F;

/**
 * @return a typed number, or nothing when the text is not a whole finite number
 **/
std::optional<float> number(std::string_view text) {
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
    immediate::Retained& retained = context_->retain(id);

    bool changed = false;
    if (context_->typing == id) {
        context_->typingDrawn = true;
        // a typed value is taken as the field lets go, so a half typed number never reaches
        // the caller
        if (context_->letGo || context_->disabled > 0 || (context_->input.pressed && !reaction.hovered)) {
            const std::optional<float> typed = context_->escaped ? std::nullopt : number(context_->edit.text());
            if (typed && *typed != *value) {
                *value = *typed;
                changed = true;
            }
            context_->focus(0);
            context_->itemFinished = true;
        }
    } else if (reaction.held) {
        // a press becomes a drag once it has travelled, and the pixels spent deciding are not
        // applied. One that never travelled is a click, which types into the field
        if (!retained.dragging && std::abs(context_->input.cursor.x - context_->pressAt.x) > dragThreshold) {
            retained.dragging = true;
        }
        if (retained.dragging && std::isfinite(context_->drag.x) && context_->drag.x != 0.0F) {
            const float wanted = *value + (context_->drag.x * speed);
            if (std::isfinite(wanted) && wanted != *value) {
                *value = wanted;
                changed = true;
            }
        }
        if (context_->input.released) {
            if (retained.dragging) {
                context_->itemFinished = true;
            } else if (reaction.clicked) {
                context_->focus(id);
                context_->typingDrawn = true;
                context_->edit.text(std::format("{}", *value));
                context_->edit.selectAll();
            }
            retained.dragging = false;
        }
    } else {
        retained.dragging = false;
    }

    const bool focused = context_->typing == id;
    const glm::vec2 low(min.x + context_->dressing.padding, min.y + context_->dressing.borderWidth);
    const glm::vec2 high(max.x - context_->dressing.padding, max.y - context_->dressing.borderWidth);
    if (focused) {
        paint::plateBox(context_->canvas, min, max, context_->dressing.radius, context_->dressing.borderWidth,
            {.inside = context_->dressing.widget, .outline = context_->dressing.focus});
    } else {
        paint::fillBox(context_->canvas, min, max, context_->dressing.radius,
            context_->eased(id, context_->face(reaction.held && retained.dragging, reaction.hovered)));
    }
    if (high.x <= low.x) {
        return changed;
    }
    context_->canvas->clip(low, high);
    if (focused) {
        context_->drawEdit(low, high);
    } else {
        const glm::vec2 room(high.x - low.x, high.y - low.y);
        context_->label(label, low, room, context_->ink(context_->dressing.secondaryText));
        const std::string shown = std::format("{:.3f}", *value);
        context_->label(shown, glm::vec2(high.x - context_->measure(shown), low.y), room, context_->ink(context_->dressing.text));
    }
    context_->canvas->unclip();
    return changed;
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

};  // namespace v3d::ui
