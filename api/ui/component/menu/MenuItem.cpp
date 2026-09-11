/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "MenuItem.h"
#include "Menu.h"

#include <string>
#include <type_traits>
#include <variant>

namespace v3d::ui::component {

MenuItem::MenuItem(menu::ItemType type, const std::string& label) :
    Component(component::Type::MenuItem),
    label_(label), type_(type), checked_(false), hasValue_(false) {
}

void MenuItem::label(const std::string& str) {
    label_ = str;
}

void MenuItem::submenu(const boost::shared_ptr<Menu>& sub) {
    submenu_ = sub;
    sub->parent(menu_);
}

std::string_view MenuItem::label() const {
    return label_;
}

/**
 **/
std::string MenuItem::text() const {
    if (!hasValue_) {
        return label_;
    }
    std::string value = std::visit([](auto&& held) -> std::string {
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
void MenuItem::value(const v3d::event::EventData& v) {
    value_ = v;
    hasValue_ = true;
}

/**
 **/
boost::optional<v3d::event::EventData> MenuItem::value() const {
    if (!hasValue_) {
        return boost::none;
    }
    return value_;
}

boost::shared_ptr<Menu> MenuItem::submenu(void) const {
    return submenu_;
}

void MenuItem::event(const v3d::event::Event &destination) {
    event_ = destination;
    event_.type(v3d::event::Type::Destination);
}

v3d::event::Event MenuItem::event() const {
    return event_;
}

void MenuItem::menu(const boost::weak_ptr<Menu>& m) {
    menu_ = m;
}

boost::weak_ptr<Menu> MenuItem::menu() {
    return menu_;
}

menu::ItemType MenuItem::itemType() const {
    return type_;
}

/**
 **/
void MenuItem::checked(bool on) {
    checked_ = on;
}

/**
 **/
bool MenuItem::checked() const {
    return checked_;
}

};  // namespace v3d::ui::component
