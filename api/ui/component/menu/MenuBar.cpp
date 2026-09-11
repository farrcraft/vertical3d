/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "MenuBar.h"

#include <api/ui/component/Type.h>

#include <cstddef>
#include <string>
#include <vector>

namespace v3d::ui::component {

/**
 **/
MenuBar::MenuBar() : Component(component::Type::MenuBar), hover_(-1), open_(-1) {
}

/**
 **/
void MenuBar::add(const std::string& label, const boost::shared_ptr<Menu>& menu) {
    labels_.push_back(label);
    // nothing is hit until a renderer has said where the label went
    bounds_.push_back(v3d::type::geometry::Bound2D(0.0f, 0.0f, 0.0f, 0.0f));
    menus_.push_back(menu);
}

/**
 **/
void MenuBar::place(std::size_t index, const glm::vec2& position, const glm::vec2& size) {
    bounds_[index] = v3d::type::geometry::Bound2D(position, size);
}

/**
 **/
v3d::type::geometry::Bound2D MenuBar::bound(std::size_t index) const {
    return bounds_[index];
}

/**
 **/
std::size_t MenuBar::count() const noexcept {
    return menus_.size();
}

/**
 **/
const std::string& MenuBar::label(std::size_t index) const {
    return labels_[index];
}

/**
 **/
boost::shared_ptr<Menu> MenuBar::menu(std::size_t index) const {
    return menus_[index];
}

/**
 **/
int MenuBar::hover() const noexcept {
    return hover_;
}

/**
 **/
int MenuBar::open() const noexcept {
    return open_;
}

/**
 **/
void MenuBar::open(int index) {
    panels_.clear();
    if (index < 0 || static_cast<std::size_t>(index) >= menus_.size()) {
        open_ = -1;
        return;
    }
    open_ = index;
    panels_.push_back(menus_[index]);
    // nothing in the panel is under the cursor yet, and an item left active by the last
    // time it was open would be highlighted where the cursor is not
    panels_.front()->active(-1);
}

/**
 **/
void MenuBar::close() {
    open(-1);
}

/**
 **/
bool MenuBar::active() const noexcept {
    return open_ >= 0;
}

/**
 **/
const std::vector<boost::shared_ptr<Menu>>& MenuBar::panels() const noexcept {
    return panels_;
}

/**
 **/
bool MenuBar::within(const Component& component, const glm::vec2& cursor) {
    v3d::type::geometry::Bound2D bound = component.bound();
    return bound.intersect(cursor);
}

/**
 **/
int MenuBar::labelAt(const glm::vec2& cursor) const {
    for (std::size_t index = 0; index < bounds_.size(); index++) {
        v3d::type::geometry::Bound2D bound = bounds_[index];
        if (bound.intersect(cursor)) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

/**
 **/
int MenuBar::itemAt(const boost::shared_ptr<Menu>& panel, const glm::vec2& cursor) {
    for (std::size_t index = 0; index < panel->count(); index++) {
        const boost::shared_ptr<MenuItem>& item = (*panel)[index];
        if (item && within(*item, cursor)) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

/**
 **/
void MenuBar::truncate(std::size_t depth) {
    while (panels_.size() > depth) {
        panels_.back()->active(-1);
        panels_.pop_back();
    }
}

/**
 **/
void MenuBar::descend(std::size_t depth, const boost::shared_ptr<MenuItem>& item) {
    truncate(depth + 1);
    if (item->itemType() != menu::ItemType::Submenu || !item->submenu()) {
        return;
    }
    panels_.push_back(item->submenu());
    panels_.back()->active(-1);
}

/**
 **/
bool MenuBar::motion(const glm::vec2& cursor) {
    // deepest first, because a flyout is drawn over the panel it came out of and so takes
    // the cursor where the two overlap
    for (std::size_t depth = panels_.size(); depth > 0; depth--) {
        const boost::shared_ptr<Menu>& panel = panels_[depth - 1];
        if (!within(*panel, cursor)) {
            continue;
        }
        const int index = itemAt(panel, cursor);
        panel->active(index);
        if (index >= 0) {
            descend(depth - 1, (*panel)[static_cast<std::size_t>(index)]);
        }
        return true;
    }

    if (!within(*this, cursor)) {
        hover_ = -1;
        return false;
    }
    hover_ = labelAt(cursor);
    if (open_ >= 0) {
        if (hover_ >= 0 && hover_ != open_) {
            // sliding across the strip opens the menu the cursor reaches
            open(hover_);
        } else {
            // the cursor is back on the strip, so it has left every flyout under it
            truncate(1);
        }
    }
    return true;
}

/**
 **/
bool MenuBar::press(const glm::vec2& cursor) {
    for (std::size_t depth = panels_.size(); depth > 0; depth--) {
        const boost::shared_ptr<Menu>& panel = panels_[depth - 1];
        if (!within(*panel, cursor)) {
            continue;
        }
        const int index = itemAt(panel, cursor);
        if (index < 0) {  // the panel's own padding, which dismisses nothing
            return true;
        }
        const boost::shared_ptr<MenuItem> item = (*panel)[static_cast<std::size_t>(index)];
        // a press can arrive without a motion having crossed the item first, so the
        // highlight and the flyout's placement are set here rather than assumed
        panel->active(index);
        if (item->itemType() == menu::ItemType::Submenu) {
            descend(depth - 1, item);
        } else {
            panel->dispatch(item);
            close();
        }
        return true;
    }

    if (within(*this, cursor)) {
        const int index = labelAt(cursor);
        if (index >= 0) {
            open(index == open_ ? -1 : index);
        }
        return true;
    }

    if (active()) {
        close();
        return true;
    }
    return false;
}

/**
 **/
boost::shared_ptr<MenuItem> MenuBar::find(const boost::shared_ptr<Menu>& menu, const std::string& command) {
    for (std::size_t index = 0; index < menu->count(); index++) {
        const boost::shared_ptr<MenuItem>& item = (*menu)[index];
        if (!item) {
            continue;
        }
        // an item is only bound when its config gave both a command and a context, and
        // Event::str() dereferences the context
        const v3d::event::Event event = item->event();
        if (event.context() && event.str() == command) {
            return item;
        }
        if (item->submenu()) {
            boost::shared_ptr<MenuItem> found = find(item->submenu(), command);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

/**
 **/
boost::shared_ptr<MenuItem> MenuBar::find(const std::string& command) const {
    for (const boost::shared_ptr<Menu>& menu : menus_) {
        boost::shared_ptr<MenuItem> found = find(menu, command);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

};  // namespace v3d::ui::component
