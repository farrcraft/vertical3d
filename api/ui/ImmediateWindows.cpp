/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/geometry/Bound2D.h>
#include <api/ui/Immediate.h>
#include <api/ui/component/Scrollbar.h>
#include <api/ui/immediate/Context.h>
#include <api/ui/paint/Painter.h>

#include <algorithm>
#include <string>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui {

bool Immediate::window(const std::string& title, const v3d::type::geometry::Bound2D::Area& area, float alpha) {
    const glm::vec2& position = area.position;
    const glm::vec2& size = area.size;
    if (context_->canvas == nullptr) {
        return false;
    }
    const Id id = context_->identify(title);
    immediate::Retained& retained = context_->retain(id);

    // the title bar both folds the window and moves it, told apart by whether the press
    // travelled. The move is applied before the window is placed, so a dragged
    // window is under the cursor on the frame it moved rather than the one after. The
    // pixels spent deciding are not applied, so a drag lags by the threshold once
    if (context_->active != id) {
        retained.dragging = false;
    } else {
        if (!retained.dragging && glm::length(context_->input.cursor - context_->pressAt) > dragThreshold) {
            retained.dragging = true;
        }
        if (retained.dragging) {
            retained.offset += context_->drag;
        }
    }

    // the drag is kept as a displacement, not a position, so a window that its caller moves
    // every frame still follows the caller. The title bar is kept on the canvas, because the
    // bar is the only way to drag a window back
    const glm::vec2 room = glm::max(glm::vec2(static_cast<float>(context_->canvas->width()) - size.x,
        static_cast<float>(context_->canvas->height()) - context_->dressing.barHeight), glm::vec2(0.0F, 0.0F));
    const glm::vec2 min = glm::clamp(position + retained.offset, glm::vec2(0.0F, 0.0F), room);
    retained.offset = min - position;

    const glm::vec2 barMax(min.x + size.x, min.y + context_->dressing.barHeight);
    const glm::vec2 max = retained.collapsed ? barMax : min + size;

    glm::vec4 background = context_->dressing.panel;
    background.a *= std::clamp(alpha, 0.0F, 1.0F);
    paint::plateBox(context_->canvas, min, max, context_->dressing.radius, context_->dressing.borderWidth, {.inside = background, .outline = context_->dressing.border});
    paint::fillBox(context_->canvas, min + glm::vec2(context_->dressing.borderWidth, context_->dressing.borderWidth),
        glm::vec2(barMax.x - context_->dressing.borderWidth, barMax.y), context_->dressing.radius, context_->dressing.titleBar);
    context_->title(title, min + glm::vec2(context_->dressing.padding, 0.0F), glm::vec2(size.x, context_->dressing.barHeight), context_->dressing.text);

    // a press that stayed put folds; one that moved has already moved the window this
    // frame and does not fold it as well
    const immediate::Reaction reaction = context_->interact(id, min, barMax);
    if (reaction.clicked && !retained.dragging) {
        retained.collapsed = !retained.collapsed;
    }
    // a window takes the cursor from everything drawn before it, so a click meant for the
    // window on top does not reach what it covers. What goes in this one is drawn after
    // and takes it back
    if (paint::inside(min, max, context_->input.cursor)) {
        context_->hovering = id;
    }

    context_->openBody({.id = id, .scroll = context_->identify(title + " scrollbar"), .min = min, .max = max, .top = barMax.y},
        !retained.collapsed);
    return !retained.collapsed;
}

void Immediate::region(const std::string& name, const v3d::type::geometry::Bound2D::Area& area, float alpha) {
    if (context_->canvas == nullptr) {
        return;
    }
    const Id id = context_->identify(name);
    const glm::vec2 min = area.position;
    const glm::vec2 max = area.position + area.size;
    if (alpha > 0.0F) {
        glm::vec4 background = context_->dressing.panel;
        background.a *= std::min(alpha, 1.0F);
        paint::fillBox(context_->canvas, min, max, 0.0F, background);
    }
    // a region takes the cursor from what was drawn before it, as a window does
    if (paint::inside(min, max, context_->input.cursor)) {
        context_->hovering = id;
    }
    context_->openBody({.id = id, .scroll = context_->identify(name + " scrollbar"), .min = min, .max = max, .top = min.y}, true);
}

void Immediate::endRegion() {
    endWindow();
}

void immediate::Context::openBody(const Body& body, bool shown) {
    window.open = true;
    window.margin = row.margin;
    window.right = row.right;
    window.id = body.id;
    window.scroll = body.scroll;
    row.margin = body.min.x + dressing.padding;
    row.right = body.max.x - dressing.padding;
    row.penY = body.top + dressing.spacing;
    row.sameLine = false;
    window.clipped = false;
    window.scrolls = false;
    if (!shown) {
        return;
    }

    Retained& retained = retain(body.id);
    window.bodyMin = glm::vec2(body.min.x + dressing.borderWidth, body.top);
    window.bodyMax = glm::vec2(body.max.x - dressing.borderWidth, body.max.y - dressing.borderWidth);
    window.contentTop = body.top + dressing.spacing;

    // whether there is a bar is decided by what the frame before this one drew, because how
    // tall the content is is only known once it has been drawn
    const float view = std::max(window.bodyMax.y - window.contentTop, 0.0F);
    window.scrolls = retained.content > view;
    if (window.scrolls) {
        row.right -= dressing.scrollbarWidth + dressing.spacing;
        retained.scroll = std::clamp(retained.scroll, 0.0F, retained.content - view);
    } else {
        retained.scroll = 0.0F;
    }

    row.penY = window.contentTop - retained.scroll;
    canvas->clip(window.bodyMin, window.bodyMax);
    window.clipped = true;

    // the wheel turns the region the cursor is over, and one drawn later is over one drawn
    // before it, so the last to claim the cursor keeps it
    if (paint::inside(body.min, body.max, input.cursor)) {
        wheeled = body.id;
    }
}

void Immediate::endWindow() {
    if (!context_->window.open) {
        return;
    }
    if (context_->window.clipped) {
        context_->canvas->unclip();
        context_->window.clipped = false;

        context_->closeScroll({.id = context_->window.id, .scroll = context_->window.scroll, .contentTop = context_->window.contentTop, .bodyMax = context_->window.bodyMax},
            context_->window.scrolls ? immediate::Bar::Always : immediate::Bar::Never);
    }
    context_->row.margin = context_->window.margin;
    context_->row.right = context_->window.right;
    context_->window.open = false;
    context_->window.scrolls = false;
    context_->row.sameLine = false;
}

void immediate::Context::closeScroll(const Scrolled& region, Bar bar) {
    const Id id = region.id;
    const Id scroll = region.scroll;
    const float contentTop = region.contentTop;
    const glm::vec2& bodyMax = region.bodyMax;
    Retained& retained = retain(id);
    // how tall what was drawn came to. The pen has the scroll taken out of it and the gap
    // after the last row left in, so both go back before it is a height
    retained.content = std::max(row.penY + retained.scroll - dressing.spacing - contentTop, 0.0F);

    const float view = std::max(bodyMax.y - contentTop, 0.0F);
    const float span = std::max(retained.content - view, 0.0F);
    if (bar == Bar::Always || (bar == Bar::WhenNeeded && span > 0.0F)) {
        scrollbar(scroll, glm::vec2(bodyMax.x - dressing.scrollbarWidth, contentTop), bodyMax, view, span,
            &retained.scroll);
    }
    if (wheeled == id && input.wheel != 0.0F) {
        // a notch away from the reader shows what is above, which is a smaller offset
        retained.scroll = std::clamp(retained.scroll - (input.wheel * dressing.lineHeight * wheelRows), 0.0F, span);
    }
}

void immediate::Context::scrollbar(Id id, const glm::vec2& min, const glm::vec2& max, float view,
    float span, float* scroll) {
    const float track = max.y - min.y;
    if (track <= 0.0F || view <= 0.0F) {
        return;
    }

    const float length = component::thumbLength(track, view, view + span);

    const Reaction reaction = interact(id, min, max);
    if (reaction.held && track - length > 0.0F) {
        *scroll = component::dragOffset(track, length, input.cursor.y - min.y, span);
    }

    glm::vec4 grip = dressing.thumb;
    if (reaction.held) {
        grip = dressing.highlight;
    } else if (reaction.hovered) {
        grip = dressing.hover;
    }

    const float start = component::thumbStart({.track = track, .length = length, .offset = *scroll, .span = span});
    paint::fillBox(canvas, min, max, dressing.radius, dressing.track);
    paint::fillBox(canvas, glm::vec2(min.x, min.y + start), glm::vec2(max.x, min.y + start + length),
        dressing.radius, grip);
}

};  // namespace v3d::ui
