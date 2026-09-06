/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Menu.h"

namespace v3d::ui::component {
Menu::Menu(const boost::shared_ptr<entt::dispatcher>& dispatcher) :
    Component(component::Type::MENU), dispatcher_(dispatcher), active_(-1) {
}

bool Menu::navigate(Navigation direction, bool /* wrap */) {
    if (direction == Navigation::UnselectItem) {
    } else if (direction == Navigation::SelectItem) {
    } else if (direction == Navigation::NextItem) {
    } else if (direction == Navigation::PreviousItem) {
    } else if (direction == Navigation::HierarchyUp) {
    } else if (direction == Navigation::HierarchyDown) {
    } else {
        return false;
    }
    return true;
}

/**
 **/
void Menu::parent(boost::weak_ptr<Menu> p) {
    parent_ = p;
}

bool Menu::hasParent() const {
    boost::shared_ptr<Menu> menu = parent_.lock();
    if (menu) {
        return true;
    }
    return false;
}

boost::shared_ptr<Menu> Menu::level() const {
    return level_.lock();
}

void Menu::level(boost::weak_ptr<Menu> m) {
    level_ = m;
}

void Menu::addItem(boost::shared_ptr<MenuItem> item) {
    items_.push_back(item);
}

boost::shared_ptr<MenuItem> Menu::active() const {
    if (active_ < 0 || static_cast<size_t>(active_) >= items_.size()) {
        return nullptr;
    }
    return items_[active_];
}

void Menu::active(int idx) {
    if (idx < 0 || static_cast<size_t>(idx) >= items_.size()) {
        active_ = -1;
        return;
    }
    active_ = idx;
}

bool Menu::next() {
    boost::shared_ptr<Menu> lvl = level();
    if (!lvl || lvl->items_.empty()) {
        return false;
    }
    lvl->active_++;
    if (lvl->active_ < 0 || static_cast<size_t>(lvl->active_) >= lvl->items_.size()) {  // wrap around
        lvl->active_ = 0;
        return false;
    }
    return true;
}

bool Menu::previous() {
    boost::shared_ptr<Menu> lvl = level();
    if (!lvl || lvl->items_.empty()) {
        return false;
    }
    if (lvl->active_ < 0 || static_cast<size_t>(lvl->active_) >= lvl->items_.size()) {  // wrap around
        lvl->active_ = 0;
        return false;
    }
    if (lvl->active_ > 0) {
        lvl->active_--;
    } else {
        lvl->active_ = static_cast<int>(lvl->items_.size()) - 1;
    }
    return true;
}

bool Menu::up() {
    boost::shared_ptr<Menu> lvl = level();
    if (!lvl) {
        return false;
    }
    if (lvl->parent_.expired()) {
        return false;
    }
    level_ = lvl->parent_;
    return true;
}

bool Menu::down() {
    boost::shared_ptr<Menu> lvl = level();
    if (!lvl) {
        return false;
    }
    boost::shared_ptr<MenuItem> item = lvl->active();
    if (!item) {
        return false;
    }
    boost::shared_ptr<Menu> sm = item->submenu();
    if (sm) {
        level_ = sm;
        return true;
    }
    return false;
}

size_t Menu::size() const {
    return items_.size();
}

boost::shared_ptr<MenuItem>& Menu::operator[](size_t i) {
    assert(i < items_.size());
    return items_[i];
}

/**
 **/
bool Menu::dispatch(const boost::shared_ptr<MenuItem>& item) const {
    v3d::event::Event event = item->event();
    // an item is only bound to an event when its config gave both a command and a context.
    // Event::str() dereferences the context, so an unbound event must never be sent.
    if (!dispatcher_ || !event.context()) {
        return false;
    }
    boost::optional<v3d::event::EventData> value = item->value();
    if (value) {
        event.data(value.get());
    }
    dispatcher_->trigger(event);
    return true;
}

void Menu::activate() {
    boost::shared_ptr<Menu> lvl = level();
    if (!lvl) {
        return;
    }
    boost::shared_ptr<MenuItem> item = lvl->active();
    if (item) {
        if (item->type() == menu::ItemType::Submenu && item->submenu()) {  // menu item has a submenu so activate the submenu
            down();
        } else if (item->type() == menu::ItemType::Action ||
            item->type() == menu::ItemType::Check ||
            item->type() == menu::ItemType::Radio) {  // menu item represents a command so send the bound event
            dispatch(item);
        } else if (item->type() == menu::ItemType::Input ||
            item->type() == menu::ItemType::NumericInput ||
            item->type() == menu::ItemType::KeyInput) {
            // the ui needs to capture all input until the next ui activation (e.g. another select menu command bound
            // event is received), and then dispatch(item) with the captured value. Nothing captures input yet, so
            // activating an input item does nothing rather than sending a stale value.
        }
    }
}

};  // namespace v3d::ui::component
