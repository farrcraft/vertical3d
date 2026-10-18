/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SelectList.h"

#include <api/event/Event.h>
#include <api/event/Type.h>
#include <api/ui/Component.h>

#include "Type.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace v3d::ui::component {

const int SelectList::none;

SelectList::SelectList() :
    Component(Type::SelectList),
    offset_(0.0F),
    rowHeight_(0.0F),
    widest_(-1.0F),
    measured_(0),
    selected_(none) {
    // a control is pickable and focusable from the start; a plain component is neither, so a
    // panel laid over a scene lets presses through
    pickable(true);
    focusable(true);
}

void SelectList::items(const std::vector<std::string>& rows) {
    items_ = rows;
    widest_ = -1.0F;
    measured_ = 0;
    selected(selected_);
    offset(offset_);
}

void SelectList::widest(const Measured& measured) noexcept {
    const std::uint64_t measure = measured.measure;
    widest_ = measured.width;
    measured_ = measure;
}

float SelectList::widest(std::uint64_t measure) const noexcept {
    return measure == measured_ ? widest_ : -1.0F;
}

const std::vector<std::string>& SelectList::items() const noexcept {
    return items_;
}

void SelectList::selected(int index) {
    selected_ = index >= 0 && index < static_cast<int>(items_.size()) ? index : none;
}

int SelectList::selected() const noexcept {
    return selected_;
}

std::string_view SelectList::selection() const {
    if (selected_ == none) {
        return std::string_view();
    }
    return items_[static_cast<std::size_t>(selected_)];
}

void SelectList::offset(float pixels) {
    // what the box does not show, which is nothing until something has drawn one
    const float hidden = std::max(content() - size().y, 0.0F);
    offset_ = std::clamp(pixels, 0.0F, hidden);
}

float SelectList::offset() const noexcept {
    return offset_;
}

void SelectList::rowHeight(float height) {
    rowHeight_ = std::max(height, 0.0F);
}

float SelectList::rowHeight() const noexcept {
    return rowHeight_;
}

float SelectList::content() const noexcept {
    return rowHeight_ * static_cast<float>(items_.size());
}

int SelectList::at(const glm::vec2& point) const {
    if (rowHeight_ <= 0.0F || items_.empty()) {
        return none;
    }
    const glm::vec2 min = position();
    const glm::vec2 max = min + size();
    if (point.x < min.x || point.x >= max.x || point.y < min.y || point.y >= max.y) {
        return none;
    }
    const int row = static_cast<int>((point.y - min.y + offset_) / rowHeight_);
    return row < static_cast<int>(items_.size()) ? row : none;
}

void SelectList::event(const v3d::event::Event& destination) {
    event_ = destination;
    // marked as a Destination event here, not by the caller, because a listener that accepts
    // only Destination events drops an unmarked one
    event_.type(v3d::event::Type::Destination);
}

v3d::event::Event SelectList::event() const {
    return event_;
}

};  // namespace v3d::ui::component
