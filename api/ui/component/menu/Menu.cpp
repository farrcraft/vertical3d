/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Menu.h"

namespace v3d::ui::component {
Menu::Menu(const boost::shared_ptr<entt::dispatcher>& dispatcher) :
    Component(component::Type::MENU), dispatcher_(dispatcher), active_(-1) {
}

/**
 **/
void Menu::parent(boost::weak_ptr<Menu> p) {
    parent_ = p;
}

bool Menu::hasParent() const {
    return !parent_.expired();
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
    // a capture has the input, so navigation does not move under it
    if (capture_) {
        return false;
    }
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
    // a capture has the input, so navigation does not move under it
    if (capture_) {
        return false;
    }
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
    // backing out of a capture is what it means here, rather than leaving the level the
    // item being captured into sits on
    if (capture_) {
        cancel();
        return true;
    }
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
    if (capture_) {
        return false;
    }
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

bool Menu::capturing() const {
    return capture_ != nullptr;
}

/**
 **/
void Menu::cancel() {
    capture_.reset();
}

/**
 **/
bool Menu::capture(const v3d::event::EventData& value) {
    if (!capture_) {
        return false;
    }
    capture_->value(value);

    // a binding is one key, so the first one given is the whole answer
    if (capture_->type() == menu::ItemType::KeyInput) {
        const boost::shared_ptr<MenuItem> item = capture_;
        capture_.reset();
        dispatch(item);
    }
    return true;
}

/**
 **/
void Menu::activate() {
    // an activation arriving while one is open is what ends a capture, whichever level
    // the item being captured into belongs to
    if (capture_) {
        const boost::shared_ptr<MenuItem> item = capture_;
        capture_.reset();
        dispatch(item);
        return;
    }

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
            capture_ = item;
        }
    }
}

};  // namespace v3d::ui::component
