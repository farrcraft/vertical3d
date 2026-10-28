/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Immediate.h>
#include <api/ui/input/LineEdit.h>
#include <api/ui/paint/Painter.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

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
        }
    }

    const bool focused = context_->typing == id;
    paint::plateBox(context_->canvas, min, max, context_->dressing.radius, context_->dressing.borderWidth,
        {.inside = context_->face(false, reaction.hovered && !focused), .outline = focused ? context_->dressing.focus : context_->dressing.border});
    if (high.x <= low.x) {
        return changed;
    }
    context_->canvas->clip(low, high);
    if (!focused && value->empty()) {
        context_->label(label, low, glm::vec2(room, size.y - (context_->dressing.borderWidth * 2.0F)), context_->dressing.placeholder);
        context_->canvas->unclip();
        return changed;
    }

    // the line slides left when the caret would be past the far edge, so a full field can
    // still be typed into
    const std::string_view line = focused ? context_->edit.text() : std::string_view(*value);
    const float caret = focused ? context_->measure(line.substr(0, context_->edit.caret())) : 0.0F;
    const float pen = low.x - std::max(caret - room, 0.0F);
    if (focused && context_->edit.selected()) {
        // the highlight goes behind the line, which is drawn whole in one colour, so the
        // runs either side of it cannot measure differently from the prefix that places the
        // caret
        const float from = context_->measure(line.substr(0, std::min(context_->edit.anchor(), context_->edit.caret())));
        const float to = context_->measure(line.substr(0, std::max(context_->edit.anchor(), context_->edit.caret())));
        paint::fillBox(context_->canvas, glm::vec2(pen + from, low.y), glm::vec2(pen + to, high.y), 0.0F, context_->dressing.highlight);
    }
    context_->label(std::string(line), glm::vec2(pen, low.y), glm::vec2(room, high.y - low.y), context_->ink(context_->dressing.text));
    if (focused) {
        const float stem = std::max(context_->dressing.borderWidth, 1.0F);
        context_->canvas->rect(glm::vec2(pen + caret, low.y), glm::vec2(pen + caret + stem, high.y), context_->dressing.caret);
    }
    context_->canvas->unclip();
    return changed;
}

};  // namespace v3d::ui
