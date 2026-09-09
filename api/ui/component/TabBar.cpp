/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TabBar.h"

#include <vector>

#include <boost/make_shared.hpp>

#include "../Painter.h"

namespace v3d::ui::component {

const int TabBar::none;

TabBar::TabBar() :
    Component(Type::TabBar),
    selected_(0) {
    // a control exists to be driven, so it asks for the press and the focus that a panel
    // laid over a scene must not take - ADR-0034 and ADR-0040
    pickable(true);
    focusable(true);
}

std::vector<boost::shared_ptr<TabPage>> TabBar::pages() const {
    std::vector<boost::shared_ptr<TabPage>> found;
    for (const boost::shared_ptr<Component>& child : children()) {
        const boost::shared_ptr<TabPage> page = boost::dynamic_pointer_cast<TabPage>(child);
        if (page) {
            found.push_back(page);
        }
    }
    return found;
}

void TabBar::selected(int index) {
    const int count = static_cast<int>(pages().size());
    selected_ = index >= 0 && index < count ? index : none;
}

int TabBar::selected() const noexcept {
    return selected_;
}

boost::shared_ptr<TabPage> TabBar::page() const {
    const std::vector<boost::shared_ptr<TabPage>> held = pages();
    // the first page is what a bar shows until something chose another, so an index that
    // was set before the pages were added still lands on one
    if (held.empty()) {
        return nullptr;
    }
    if (selected_ < 0 || selected_ >= static_cast<int>(held.size())) {
        return nullptr;
    }
    return held[static_cast<std::size_t>(selected_)];
}

void TabBar::tabs(const std::vector<v3d::type::Bound2D>& boxes) {
    tabs_ = boxes;
}

const std::vector<v3d::type::Bound2D>& TabBar::tabs() const noexcept {
    return tabs_;
}

int TabBar::at(const glm::vec2& point) const {
    for (std::size_t index = 0; index < tabs_.size(); index++) {
        if (inside(tabs_[index].position(), tabs_[index].position() + tabs_[index].size(), point)) {
            return static_cast<int>(index);
        }
    }
    return none;
}

};  // namespace v3d::ui::component
