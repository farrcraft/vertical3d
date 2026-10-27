/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Immediate.h>
#include <api/ui/paint/Painter.h>

#include <algorithm>
#include <cstddef>
#include <string>

#include <glm/vec2.hpp>

#include "Context.h"

namespace v3d::ui {

bool Immediate::tabBar(const std::string& id) {
    if (context_->canvas == nullptr) {
        return false;
    }
    context_->tabs.id = context_->identify(id);
    context_->tabs.open = true;
    context_->tabs.index = 0;
    context_->tabs.changed = false;
    context_->tabs.taken = false;
    // the strip takes a row of its own and the pen goes past it, so what a selected tab
    // holds is drawn under the whole strip rather than beside the next tab
    context_->tabs.top = context_->row.penY;
    context_->tabs.pen = context_->row.margin;
    context_->row.penY = context_->tabs.top + context_->dressing.barHeight;
    context_->canvas->rect(glm::vec2(context_->row.margin, context_->row.penY),
        glm::vec2(context_->row.right, context_->row.penY + immediate::Context::ruleWidth), context_->dressing.rule);
    context_->row.penY += immediate::Context::ruleWidth + context_->dressing.spacing;
    context_->row.sameLine = false;
    return true;
}

bool Immediate::tab(const std::string& label) {
    if (context_->canvas == nullptr || !context_->tabs.open) {
        return false;
    }
    const immediate::Retained& retained = context_->retain(context_->tabs.id);
    const unsigned int index = context_->tabs.index++;
    const glm::vec2 size(context_->measure(label) + (context_->dressing.padding * 2.0F), context_->dressing.barHeight);
    const glm::vec2 min(context_->tabs.pen, context_->tabs.top);
    context_->tabs.pen += size.x + context_->dressing.spacing;

    const Id id = context_->identify(label);
    const immediate::Reaction reaction = context_->interact(id, min, min + size);
    if (reaction.clicked) {
        context_->tabs.wanted = index;
        context_->tabs.changed = true;
    }
    const bool selected = retained.tab == index;
    context_->tabs.taken = context_->tabs.taken || selected;

    // a tab at rest is drawn as a tab, not as a widget's face
    const bool lit = context_->disabled == 0 && (selected || reaction.hovered);
    paint::fillBox(context_->canvas, min, min + size, context_->dressing.radius, lit ? context_->face(selected, reaction.hovered) : context_->dressing.tab);
    context_->label(label, min + glm::vec2(context_->dressing.padding, 0.0F), size,
        selected ? context_->dressing.activeText : context_->dressing.text);
    return selected;
}

void Immediate::endTabBar() {
    if (!context_->tabs.open) {
        return;
    }
    if (context_->tabs.changed) {
        context_->retain(context_->tabs.id).tab = context_->tabs.wanted;
    } else if (!context_->tabs.taken && context_->tabs.index > 0) {
        // a strip whose selected tab is no longer there falls back to the first, so a bar
        // is never drawn with nothing chosen
        context_->retain(context_->tabs.id).tab = 0;
    }
    context_->tabs.open = false;
}

bool Immediate::table(const std::string& id, const TableShape& shape) {
    const unsigned int columns = shape.columns;
    const float height = shape.height;
    if (context_->canvas == nullptr || columns == 0) {
        return false;
    }
    context_->table.id = context_->identify(id);
    pushId(id);
    context_->table.open = true;
    context_->table.headers.clear();
    context_->table.widths.assign(columns, 0.0F);
    context_->table.left = context_->row.margin;
    context_->table.right = context_->row.right;
    context_->table.column = 0;
    context_->table.height = std::max(height, 0.0F);
    context_->table.top = context_->row.penY;
    context_->table.contentTop = context_->row.penY;
    context_->table.clipped = false;
    context_->table.scroll = context_->identify("scrollbar");
    context_->row.sameLine = false;

    // the gutter is reserved for as long as the table has a height, whether or not there
    // is anything to scroll yet. The columns then do not re-flow when a row arrives. The
    // bar can also be drawn the frame the content overflows rather than the one after, as
    // a window's has to be
    if (context_->table.height > 0.0F) {
        context_->row.right -= context_->dressing.scrollbarWidth + context_->dressing.spacing;
    }
    return true;
}

void immediate::Context::tableBody() {
    if (!table.open || table.height <= 0.0F || table.clipped) {
        return;
    }
    Retained& retained = retain(table.id);
    table.contentTop = row.penY;

    const glm::vec2 min(table.left, table.contentTop);
    const glm::vec2 max(table.right, table.top + table.height);
    const float view = std::max(max.y - min.y, 0.0F);
    retained.scroll = std::clamp(retained.scroll, 0.0F, std::max(retained.content - view, 0.0F));

    canvas->clip(min, max);
    table.clipped = true;
    row.penY = table.contentTop - retained.scroll;

    // the wheel turns the innermost region under the cursor. A table takes it from the
    // window it is drawn in, by the same rule that lets a window take it from one under it
    if (paint::inside(glm::vec2(table.left, table.top), max, input.cursor)) {
        wheeled = table.id;
    }
}

void Immediate::column(const std::string& label, float width) {
    if (!context_->table.open || context_->table.headers.size() >= context_->table.widths.size()) {
        return;
    }
    context_->table.widths[context_->table.headers.size()] = width;
    context_->table.headers.push_back(label);
}

float immediate::Context::columnStart(unsigned int index) const {
    // a column given no width of its own shares out what the named ones left
    float named = 0.0F;
    unsigned int unnamed = 0;
    for (const float width : table.widths) {
        if (width > 0.0F) {
            named += width;
        } else {
            unnamed++;
        }
    }
    const float spare = unnamed == 0 ? 0.0F
        : std::max(0.0F, (row.right - table.left - named) / static_cast<float>(unnamed));

    float start = table.left;
    for (unsigned int column = 0; column < index && column < table.widths.size(); column++) {
        start += table.widths[column] > 0.0F ? table.widths[column] : spare;
    }
    return start;
}

void Immediate::headerRow() {
    if (context_->canvas == nullptr || !context_->table.open) {
        return;
    }
    // the band runs the whole width of the table, not the width its columns were given. A
    // scrolling table's gutter is banded beside the names, not notched out of the band. The
    // two widths are the same for a table that does not scroll
    const glm::vec2 min(context_->table.left, context_->row.penY);
    const glm::vec2 max(context_->table.right, context_->row.penY + context_->dressing.lineHeight);
    paint::fillBox(context_->canvas, min, max, 0.0F, context_->dressing.titleBar);
    for (std::size_t index = 0; index < context_->table.headers.size(); index++) {
        const glm::vec2 corner(context_->columnStart(static_cast<unsigned int>(index)) + (context_->dressing.padding * 0.5F), min.y);
        context_->label(context_->table.headers[index], corner, glm::vec2(0.0F, context_->dressing.lineHeight), context_->dressing.disabledText);
    }
    context_->row.penY = max.y + context_->dressing.spacing;
    nextRow();
}

void Immediate::nextRow() {
    if (!context_->table.open) {
        return;
    }
    // the scrolled region holds the rows, so it starts at the first row: below the header
    // when there is one, and at the top of the table when there is not
    context_->tableBody();
    context_->table.column = 0;
    context_->row.margin = context_->columnStart(0);
    context_->row.sameLine = false;
}

void Immediate::nextColumn() {
    if (!context_->table.open) {
        return;
    }
    context_->table.column++;
    // sameLine() keeps the next widget on the row. Setting lastRight puts it at the column's
    // start, not wherever the last widget ended
    context_->row.sameLine = true;
    context_->row.lastRight = context_->columnStart(context_->table.column) - context_->dressing.spacing;
}

void Immediate::endTable() {
    if (!context_->table.open) {
        return;
    }
    if (context_->table.clipped) {
        context_->canvas->unclip();
        context_->table.clipped = false;

        const float bottom = context_->table.top + context_->table.height;
        context_->closeScroll({.id = context_->table.id, .scroll = context_->table.scroll, .contentTop = context_->table.contentTop,
            .bodyMax = glm::vec2(context_->table.right, bottom)}, immediate::Bar::WhenNeeded);
        // what follows the table goes under the region, not under the rows, because the rows
        // can be shorter or taller than the height the table was given
        context_->row.penY = bottom + context_->dressing.spacing;
    }
    context_->table.open = false;
    context_->row.margin = context_->table.left;
    context_->row.right = context_->table.right;
    context_->row.sameLine = false;
    popId();
}

};  // namespace v3d::ui
