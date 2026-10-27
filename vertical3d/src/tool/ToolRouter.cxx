/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "ToolRouter.h"

#include <algorithm>
#include <vector>

#include "Tool.h"

#include <glm/vec2.hpp>

namespace v3d::editor {

void ToolRouter::add(const boost::shared_ptr<Tool>& tool) {
    if (tool) {
        tools_.push_back(tool);
    }
}

void ToolRouter::view(const boost::shared_ptr<ViewPort>& view) {
    for (const boost::shared_ptr<Tool>& tool : tools_) {
        tool->view(view);
    }
}

bool ToolRouter::button(bool pressed, const glm::vec2& position) {
    if (!pressed) {
        if (!active_) {
            return false;
        }
        active_->button(1, false, position);
        active_.reset();
        return true;
    }
    // a press while a gesture is under way, as when the window lost the release, ends that
    // gesture first
    if (active_) {
        button(false, position);
    }
    const auto taker = std::find_if(tools_.begin(), tools_.end(),
        [&position](const boost::shared_ptr<Tool>& tool) { return tool->takes(position); });
    if (taker == tools_.end()) {
        return false;
    }
    active_ = *taker;
    active_->button(1, true, position);
    return true;
}

void ToolRouter::motion(const glm::vec2& position) {
    for (const boost::shared_ptr<Tool>& tool : tools_) {
        tool->motion(position);
    }
}

bool ToolRouter::dragging() const {
    return std::any_of(tools_.begin(), tools_.end(), [](const boost::shared_ptr<Tool>& tool) { return tool->dragging(); });
}

const boost::shared_ptr<Tool>& ToolRouter::active() const noexcept {
    return active_;
}

};  // namespace v3d::editor
