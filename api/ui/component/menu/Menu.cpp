/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Menu.h"

#include <api/event/Event.h>
#include <api/ui/Component.h>
#include <api/ui/component/Type.h>
#include <api/ui/input/Command.h>

#include "Item.h"
#include "Type.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>

namespace v3d::ui::component::menu {
Menu::Menu(const boost::shared_ptr<entt::dispatcher>& dispatcher) :
    Component(component::Type::Menu), dispatcher_(dispatcher), active_(-1) {
}

Menu::~Menu() {
    for (const boost::shared_ptr<Item>& item : items_) {
        if (item) {
            disown(*item);
        }
    }
}

/**
 **/
void Menu::parent(const boost::weak_ptr<Menu>& parentMenu) {
    parent_ = parentMenu;
}

bool Menu::hasParent() const {
    return !parent_.expired();
}

boost::shared_ptr<Menu> Menu::level() const {
    return level_.lock();
}

void Menu::level(const boost::weak_ptr<Menu>& current) {
    level_ = current;
}

void Menu::addItem(const boost::shared_ptr<Item>& item) {
    if (item) {
        adopt(*item);
    }
    items_.push_back(item);
}

boost::shared_ptr<Item> Menu::active() const {
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
    const boost::shared_ptr<Menu> lvl = level();
    if (!lvl || lvl->items_.empty()) {
        return false;
    }
    lvl->active_++;
    if (lvl->active_ < 0 || static_cast<size_t>(lvl->active_) >= lvl->items_.size()) {  // wrap around
        lvl->active_ = 0;
        lvl->skip(1);
        return false;
    }
    return lvl->skip(1);
}

bool Menu::previous() {
    // a capture has the input, so navigation does not move under it
    if (capture_) {
        return false;
    }
    const boost::shared_ptr<Menu> lvl = level();
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
    return lvl->skip(-1);
}

bool Menu::skip(int step) {
    // a level of nothing but separators leaves the choice where it landed, rather than
    // circling for ever
    const int count = static_cast<int>(items_.size());
    bool wrapped = false;
    for (int tries = 0; tries < count; tries++) {
        const boost::shared_ptr<Item>& item = items_[static_cast<std::size_t>(active_)];
        if (!item || !item->separator()) {
            break;
        }
        active_ += step;
        if (active_ >= count) {
            active_ = 0;
            wrapped = true;
        } else if (active_ < 0) {
            active_ = count - 1;
            wrapped = true;
        }
    }
    return !wrapped;
}

bool Menu::up() {
    // while a capture is open, up() cancels the capture and stays on the same level
    if (capture_) {
        cancel();
        return true;
    }
    const boost::shared_ptr<Menu> lvl = level();
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
    const boost::shared_ptr<Menu> lvl = level();
    if (!lvl) {
        return false;
    }
    const boost::shared_ptr<Item> item = lvl->active();
    // a disabled item, or one in a disabled menu, is not opened
    if (!item || !usable(*item)) {
        return false;
    }
    const boost::shared_ptr<Menu> sm = item->submenu();
    if (sm) {
        level_ = sm;
        return true;
    }
    return false;
}

void Menu::title(const std::string& words) {
    title_ = words;
}

const std::string& Menu::title() const noexcept {
    return title_;
}

void Menu::icon(const std::string& source) {
    if (source == icon_) {
        return;
    }
    icon_ = source;
    image_ = v3d::ui::Image();
}

std::string_view Menu::icon() const {
    return icon_;
}

const v3d::ui::Image& Menu::image() const noexcept {
    return image_;
}

void Menu::image(const v3d::ui::Image& resolved) noexcept {
    image_ = resolved;
}

void Menu::drop(bool shown) noexcept {
    drop_ = shown;
}

bool Menu::drop() const noexcept {
    return drop_;
}

void Menu::emphasis(bool set) noexcept {
    emphasis_ = set;
}

bool Menu::emphasis() const noexcept {
    return emphasis_;
}

void Menu::accent(bool tinted) noexcept {
    accent_ = tinted;
}

bool Menu::accent() const noexcept {
    return accent_;
}

void Menu::named(bool shown) noexcept {
    named_ = shown;
}

bool Menu::named() const noexcept {
    return named_;
}

size_t Menu::count() const {
    return items_.size();
}

boost::shared_ptr<Item>& Menu::operator[](size_t index) {
    assert(index < items_.size());
    return items_[index];
}

/**
 **/
bool Menu::dispatch(const boost::shared_ptr<Item>& item) const {
    v3d::event::Event event = item->event();
    // an item is bound to an event only when its config gave both a command and a context.
    // A disabled item sends nothing
    if (!usable(*item)) {
        return false;
    }
    boost::optional<v3d::event::EventData> value = item->value();
    if (value) {
        event.data(value.get());
    }
    return v3d::ui::input::send(dispatcher_.get(), event);
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
boost::shared_ptr<Item> Menu::captured() const {
    return capture_;
}

/**
 **/
bool Menu::step(int direction) {
    if (capture_) {
        return false;
    }
    const boost::shared_ptr<Menu> lvl = level();
    if (!lvl) {
        return false;
    }
    const boost::shared_ptr<Item> item = lvl->active();
    if (!item || !usable(*item) || !item->step(direction)) {
        return false;
    }
    dispatch(item);
    return true;
}

/**
 **/
boost::shared_ptr<Item> Menu::find(std::string_view command) const {
    for (const boost::shared_ptr<Item>& item : items_) {
        if (!item) {
            continue;
        }
        if (item->event().context() && item->event().name() == command) {
            return item;
        }
        if (const boost::shared_ptr<Menu> sub = item->submenu()) {
            if (boost::shared_ptr<Item> found = sub->find(command)) {
                return found;
            }
        }
    }
    return nullptr;
}

/**
 **/
bool Menu::capture(const v3d::event::EventData& value) {
    if (!capture_) {
        return false;
    }
    capture_->value(value);

    // a binding is one key, so the first value given finishes a key input
    if (capture_->itemType() == menu::ItemType::KeyInput) {
        const boost::shared_ptr<Item> item = capture_;
        capture_.reset();
        dispatch(item);
    }
    return true;
}

/**
 **/
void Menu::activate() {
    // an activation arriving while a capture is open ends it, whichever level
    // the item being captured into belongs to
    if (capture_) {
        const boost::shared_ptr<Item> item = capture_;
        capture_.reset();
        dispatch(item);
        return;
    }

    const boost::shared_ptr<Menu> lvl = level();
    if (!lvl) {
        return;
    }
    const boost::shared_ptr<Item> item = lvl->active();
    // a disabled item, or one in a disabled menu, does nothing. An input item would otherwise
    // open a capture and swallow the next key
    if (item && !usable(*item)) {
        return;
    }
    if (item) {
        if (item->itemType() == menu::ItemType::Submenu && item->submenu()) {  // menu item has a submenu so activate the submenu
            down();
        } else if (item->itemType() == menu::ItemType::Action ||
            item->itemType() == menu::ItemType::Check ||
            item->itemType() == menu::ItemType::Radio) {  // menu item represents a command so send the bound event
            dispatch(item);
        } else if (item->itemType() == menu::ItemType::Input ||
            item->itemType() == menu::ItemType::NumericInput ||
            item->itemType() == menu::ItemType::KeyInput) {
            capture_ = item;
        }
    }
}

};  // namespace v3d::ui::component::menu
