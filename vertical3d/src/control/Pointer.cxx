/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "Pointer.h"

#include <api/ui/input/Cursor.h>
#include <vertical3d/src/tool/ToolRouter.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ViewPort.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

Pointer::Pointer(const boost::shared_ptr<v3d::ui::input::Cursor>& ui, const boost::shared_ptr<AreaViews>& views,
    const boost::shared_ptr<ToolRouter>& tools, const Moved& moved) :
    ui_(ui),
    views_(views),
    tools_(tools),
    moved_(moved),
    cursor_(0.0F, 0.0F),
    uiGrab_(false) {
    if (!views_->shown().empty()) {
        activeView_ = views_->shown().front();
        tools_->view(activeView_);
    }
}

void Pointer::motion(const glm::vec2& cursor, bool snap) {
    cursor_ = cursor;

    AreaLayout& layout = *views_->layout();
    if (layout.grabbed()) {
        layout.drag(cursor_.x, cursor_.y, snap);
        place();
        return;
    }

    const bool dragging = tools_->dragging();
    overUi_ = !dragging && ui_ && ui_->motion(cursor_);
    if (overUi_) {
        return;
    }

    if (!dragging) {
        const std::size_t area = layout.areaAt(cursor_.x, cursor_.y);
        if (area < layout.areas().size()) {
            activeArea_ = layout.areas()[area].id;
        }
        const boost::shared_ptr<ViewPort> view = views_->of(area);
        if (view && view != activeView_) {
            activate(view);
        }
    }
    tools_->motion(cursor_);
}

void Pointer::button(bool pressed, unsigned int clicks) {
    AreaLayout& layout = *views_->layout();
    if (pressed) {
        // the ui is drawn over every view, so it is offered the press first
        uiGrab_ = ui_ && ui_->press(cursor_);
        if (uiGrab_) {
            return;
        }
        // a press on a divider between areas resizes them, and no tool sees it
        dividerGrab_ = (clicks >= 2 && layout.even(cursor_.x, cursor_.y)) || layout.grab(cursor_.x, cursor_.y);
        if (dividerGrab_) {
            place();
            return;
        }
        tools_->button(true, cursor_);
        return;
    }
    if (uiGrab_) {
        uiGrab_ = false;
        ui_->release(cursor_);
        return;
    }
    if (dividerGrab_) {
        dividerGrab_ = false;
        layout.release();
        return;
    }
    tools_->button(false, cursor_);
}

bool Pointer::cancel() {
    AreaLayout& layout = *views_->layout();
    if (!layout.grabbed()) {
        return false;
    }
    layout.cancel();
    place();
    return true;
}

std::optional<AreaLayout::Divider> Pointer::lit() const {
    const AreaLayout& layout = *views_->layout();
    if (layout.grabbed()) {
        return layout.held();
    }
    if (overUi_ || uiGrab_ || tools_->dragging()) {
        return std::nullopt;
    }
    return layout.divider(cursor_.x, cursor_.y);
}

std::size_t Pointer::activeArea() const noexcept {
    const AreaLayout& layout = *views_->layout();
    const std::size_t found = activeArea_ ? layout.find(*activeArea_) : layout.areas().size();
    return found < layout.areas().size() ? found : 0;
}

void Pointer::resize(const glm::vec4& region) {
    views_->layout()->resize(region);
    place();
}

const glm::vec2& Pointer::cursor() const noexcept {
    return cursor_;
}

const boost::shared_ptr<ViewPort>& Pointer::activeView() const noexcept {
    return activeView_;
}

void Pointer::place() {
    views_->sync();
    const std::vector<boost::shared_ptr<ViewPort>>& shown = views_->shown();
    if (std::ranges::find(shown, activeView_) == shown.end()) {
        // the first view shown is where the pointer starts, which is not a move
        if (!activeView_ && !shown.empty()) {
            activeView_ = shown.front();
            tools_->view(activeView_);
        } else if (!shown.empty()) {
            activate(shown.front());
        }
        return;
    }
    // a tool sized by its view, as the camera's arcball is, has to hear about the new region
    if (activeView_) {
        tools_->view(activeView_);
    }
}

void Pointer::activate(const boost::shared_ptr<ViewPort>& view) {
    activeView_ = view;
    tools_->view(activeView_);
    if (moved_) {
        moved_(activeView_);
    }
}

};  // namespace v3d::editor
