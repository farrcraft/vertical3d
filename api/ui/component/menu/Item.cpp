/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Item.h"

#include <api/event/Event.h>
#include <api/event/Type.h>
#include <api/ui/Component.h>
#include <api/ui/component/Type.h>
#include "Menu.h"
#include "Type.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace v3d::ui::component::menu {

Item::Item(menu::ItemType type, const std::string& label) :
    Component(component::Type::MenuItem),
    label_(label), type_(type), checked_(false), hasValue_(false) {
}

Item::~Item() {
    if (submenu_) {
        disown(*submenu_);
    }
}

void Item::label(const std::string& str) {
    label_ = str;
}

void Item::submenu(const boost::shared_ptr<Menu>& sub) {
    // a submenu this replaces may still be held by the app, and must not name this item
    if (submenu_) {
        disown(*submenu_);
    }
    submenu_ = sub;
    if (!sub) {
        return;
    }
    sub->parent(menu_);
    // the item becomes the submenu's parent, so disabling the item, or the menu it is in,
    // disables what the submenu holds
    adopt(*sub);
}

std::string_view Item::label() const {
    return label_;
}

/**
 **/
std::string Item::text() const {
    return label_ + shown();
}

/**
 **/
std::string Item::shown() const {
    if (!hasValue_) {
        return std::string();
    }
    return std::visit([](auto&& held) -> std::string {
        using T = std::decay_t<decltype(held)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return held;
        } else if constexpr (std::is_same_v<T, bool>) {
            return held ? "true" : "false";
        } else {
            return std::to_string(held);
        }
    }, value_);
}

/**
 **/
void Item::range(const Range& span) {
    range_ = span;
    if (hasValue_ && std::holds_alternative<int>(value_)) {
        value_ = std::clamp(std::get<int>(value_), range_.minimum, std::max(range_.minimum, range_.maximum));
    }
}

/**
 **/
const Item::Range& Item::range() const noexcept {
    return range_;
}

/**
 **/
bool Item::step(int direction) {
    if (type_ != menu::ItemType::Slider || direction == 0) {
        return false;
    }
    const int was = hasValue_ && std::holds_alternative<int>(value_) ? std::get<int>(value_) : range_.minimum;
    const int stride = direction > 0 ? range_.step : -range_.step;
    const int moved = std::clamp(was + stride, range_.minimum, std::max(range_.minimum, range_.maximum));
    const bool changed = !hasValue_ || moved != was;
    value(moved);
    return changed;
}

/**
 **/
void Item::binds(const v3d::event::Event& command) {
    binds_ = command;
}

/**
 **/
const v3d::event::Event& Item::binds() const noexcept {
    return binds_;
}

/**
 **/
void Item::value(const v3d::event::EventData& v) {
    value_ = v;
    hasValue_ = true;
}

/**
 **/
boost::optional<v3d::event::EventData> Item::value() const {
    if (!hasValue_) {
        return boost::none;
    }
    return value_;
}

boost::shared_ptr<Menu> Item::submenu(void) const {
    return submenu_;
}

void Item::event(const v3d::event::Event &destination) {
    event_ = destination;
    event_.type(v3d::event::Type::Destination);
}

v3d::event::Event Item::event() const {
    return event_;
}

void Item::menu(const boost::weak_ptr<Menu>& owner) {
    menu_ = owner;
}

boost::weak_ptr<Menu> Item::menu() {
    return menu_;
}

menu::ItemType Item::itemType() const {
    return type_;
}

/**
 **/
void Item::checked(bool on) {
    checked_ = on;
}

/**
 **/
bool Item::checked() const {
    return (type_ == menu::ItemType::Check || type_ == menu::ItemType::Radio) && checked_;
}

void Item::icon(const std::string& source) {
    if (source == icon_) {
        return;
    }
    icon_ = source;
    image_ = v3d::ui::Image();
}

std::string_view Item::icon() const {
    return icon_;
}

const v3d::ui::Image& Item::image() const noexcept {
    return image_;
}

void Item::image(const v3d::ui::Image& resolved) noexcept {
    image_ = resolved;
}

bool Item::separator() const noexcept {
    return type_ == menu::ItemType::Separator;
}

};  // namespace v3d::ui::component::menu
