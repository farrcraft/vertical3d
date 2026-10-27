/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "Pointer.h"

#include <api/ui/input/Cursor.h>
#include <vertical3d/src/tool/ToolRouter.h>
#include <vertical3d/src/view/ViewLayout.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

Pointer::Pointer(const boost::shared_ptr<v3d::ui::input::Cursor>& ui, const boost::shared_ptr<ViewLayout>& layout,
    const std::vector<boost::shared_ptr<ViewPort>>& views, const boost::shared_ptr<ToolRouter>& tools,
    const Moved& moved) :
    ui_(ui),
    layout_(layout),
    views_(views),
    tools_(tools),
    moved_(moved),
    cursor_(0.0F, 0.0F),
    uiGrab_(false) {
    if (!views_.empty()) {
        activeView_ = views_.front();
        tools_->view(activeView_);
    }
}

void Pointer::motion(const glm::vec2& cursor) {
    cursor_ = cursor;

    if (layout_->grabbed()) {
        layout_->drag(cursor_.x, cursor_.y);
        place();
        return;
    }

    const bool dragging = tools_->dragging();
    if (!dragging && ui_ && ui_->motion(cursor_)) {
        return;
    }

    if (!dragging) {
        const std::size_t index = layout_->viewAt(cursor_.x, cursor_.y);
        if (index < views_.size() && views_[index] != activeView_) {
            activeView_ = views_[index];
            tools_->view(activeView_);
            if (moved_) {
                moved_(activeView_);
            }
        }
    }
    tools_->motion(cursor_);
}

void Pointer::button(bool pressed) {
    if (pressed) {
        // the ui is drawn over every view, so it is offered the press first
        uiGrab_ = ui_ && ui_->press(cursor_);
        if (uiGrab_) {
            return;
        }
        // a press on a divider between views resizes them, and no tool sees it
        if (layout_->grab(cursor_.x, cursor_.y)) {
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
    if (layout_->grabbed()) {
        layout_->release();
        return;
    }
    tools_->button(false, cursor_);
}

void Pointer::resize(const glm::vec4& area) {
    layout_->resize(area);
    place();
}

const glm::vec2& Pointer::cursor() const noexcept {
    return cursor_;
}

const boost::shared_ptr<ViewPort>& Pointer::activeView() const noexcept {
    return activeView_;
}

void Pointer::place() {
    const std::vector<ViewLayout::View>& regions = layout_->views();
    for (std::size_t index = 0; index < views_.size() && index < regions.size(); index++) {
        views_[index]->resize(regions[index].region);
    }
    // a tool sized by its view, as the camera's arcball is, has to hear about the new region
    if (activeView_) {
        tools_->view(activeView_);
    }
}

};  // namespace v3d::editor
