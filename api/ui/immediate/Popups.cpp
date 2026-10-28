/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ui/Immediate.h>
#include <api/ui/paint/Painter.h>

#include <cstddef>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

#include "Context.h"

namespace v3d::ui {

namespace {

/**
 * How wide the mark at the right end of a combo is, as a fraction of a row.
 **/
const float comboMark = 0.4F;

};  // namespace

bool Immediate::combo(const std::string& label, int* selected, const std::vector<std::string>& items) {
    if (context_->canvas == nullptr || selected == nullptr) {
        return false;
    }
    const glm::vec2 size(context_->itemWidth(), context_->dressing.barHeight);
    const glm::vec2 min = context_->place(size);
    const glm::vec2 max = min + size;
    const Id id = context_->identify(label);
    immediate::Retained& retained = context_->retain(id);

    // a click on the list last frame is taken now, by the call that holds the caller's index
    bool changed = false;
    if (retained.chosen >= 0) {
        if (static_cast<std::size_t>(retained.chosen) < items.size() && retained.chosen != *selected) {
            *selected = retained.chosen;
            changed = true;
        }
        retained.chosen = -1;
    }

    const immediate::Reaction reaction = context_->interact(id, min, max);
    if (reaction.clicked) {
        context_->popup.id = context_->popup.id == id ? 0 : id;
    }
    const bool open = context_->popup.id == id;
    if (open) {
        context_->popup.asked = true;
        context_->popup.boxMin = min;
        context_->popup.boxMax = max;
        context_->popup.items = items;
        context_->popup.selected = *selected;
    }

    paint::fillBox(context_->canvas, min, max, context_->dressing.radius, context_->face(open || reaction.held, reaction.hovered));
    const float mark = context_->dressing.lineHeight * comboMark;
    const glm::vec2 markMin(max.x - context_->dressing.padding - mark, min.y + ((size.y - mark) * 0.5F));
    paint::fillBox(context_->canvas, markMin, markMin + glm::vec2(mark, mark), 0.0F,
        context_->ink(reaction.hovered ? context_->dressing.activeText : context_->dressing.text));
    if (*selected >= 0 && static_cast<std::size_t>(*selected) < items.size()) {
        context_->label(items[static_cast<std::size_t>(*selected)], min + glm::vec2(context_->dressing.padding, 0.0F), size,
            context_->ink(reaction.hovered ? context_->dressing.activeText : context_->dressing.text));
    }
    return changed;
}

void immediate::Context::drawPopup() {
    if (popup.id == 0) {
        return;
    }
    if (!popup.asked || canvas == nullptr) {
        // a combo that was not drawn this frame cannot hold a list open
        popup.id = 0;
        return;
    }
    const float rowHeight = dressing.lineHeight;
    const glm::vec2 min(popup.boxMin.x, popup.boxMax.y);
    const glm::vec2 max(popup.boxMax.x, min.y + (rowHeight * static_cast<float>(popup.items.size())));  // NOLINT(build/include_what_you_use) - a corner, not std::max
    paint::plateBox(canvas, min, max, 0.0F, dressing.borderWidth, {.inside = dressing.panel, .outline = dressing.border});
    if (paint::inside(min, max, input.cursor)) {
        // the list takes the cursor from what it covers, so a press between two items does
        // not reach a widget under it
        hovering = popup.id;
    }

    // the items' ids hang off the combo's, so two combos' lists do not share them
    ids.push_back(popup.id);
    bool close = false;
    for (std::size_t index = 0; index < popup.items.size(); index++) {
        const glm::vec2 entryMin(min.x, min.y + (rowHeight * static_cast<float>(index)));
        const glm::vec2 entryMax(max.x, entryMin.y + rowHeight);
        const Reaction reaction = interact(identify(std::to_string(index)), entryMin, entryMax);
        const bool chosen = static_cast<int>(index) == popup.selected;
        if (chosen || reaction.hovered) {
            paint::fillBox(canvas, entryMin, entryMax, 0.0F, chosen ? dressing.highlight : dressing.hover);
        }
        label(popup.items[index], entryMin + glm::vec2(dressing.padding, 0.0F), glm::vec2(max.x - min.x, rowHeight),
            chosen || reaction.hovered ? dressing.activeText : dressing.text);
        if (reaction.clicked) {
            retain(popup.id).chosen = static_cast<int>(index);
            close = true;
        }
    }
    ids.pop_back();

    // a press outside the list closes it. One on the combo's own box is left to the combo,
    // whose click toggles it
    if (input.pressed && !paint::inside(min, max, input.cursor) &&
        !paint::inside(popup.boxMin, popup.boxMax, input.cursor)) {
        close = true;
    }
    if (close) {
        popup.id = 0;
    }
}

};  // namespace v3d::ui
