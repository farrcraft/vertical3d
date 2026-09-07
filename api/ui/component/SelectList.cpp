/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SelectList.h"

#include <algorithm>
#include <string>
#include <vector>

namespace v3d::ui::component {

const int SelectList::none;

SelectList::SelectList() :
    Component(Type::SelectList),
    offset_(0.0f),
    rowHeight_(0.0f),
    widest_(-1.0f),
    selected_(none) {
}

void SelectList::items(const std::vector<std::string>& rows) {
    items_ = rows;
    widest_ = -1.0f;
    selected(selected_);
    offset(offset_);
}

void SelectList::widest(float width) noexcept {
    widest_ = width;
}

float SelectList::widest() const noexcept {
    return widest_;
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
    const float hidden = std::max(content() - size().y, 0.0f);
    offset_ = std::clamp(pixels, 0.0f, hidden);
}

float SelectList::offset() const noexcept {
    return offset_;
}

void SelectList::rowHeight(float height) {
    rowHeight_ = std::max(height, 0.0f);
}

float SelectList::rowHeight() const noexcept {
    return rowHeight_;
}

float SelectList::content() const noexcept {
    return rowHeight_ * static_cast<float>(items_.size());
}

int SelectList::at(const glm::vec2& point) const {
    if (rowHeight_ <= 0.0f || items_.empty()) {
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
}

v3d::event::Event SelectList::event() const {
    return event_;
}

};  // namespace v3d::ui::component
