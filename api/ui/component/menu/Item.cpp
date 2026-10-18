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
    if (!hasValue_) {
        return label_;
    }
    const std::string value = std::visit([](auto&& held) -> std::string {
        using T = std::decay_t<decltype(held)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return held;
        } else if constexpr (std::is_same_v<T, bool>) {
            return held ? "true" : "false";
        } else {
            return std::to_string(held);
        }
    }, value_);
    return label_ + value;
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
    return checked_;
}

};  // namespace v3d::ui::component::menu
