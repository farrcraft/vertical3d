/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "MenuItem.h"
#include "Menu.h"

namespace v3d::ui::component {

    MenuItem::MenuItem(menu::ItemType type, const std::string& label) :
        Component(component::Type::MENU_ITEM),
        label_(label), type_(type) {
    }

    void MenuItem::label(const std::string& str) {
        label_ = str;
    }

    void MenuItem::submenu(boost::shared_ptr<Menu> sub) {
        submenu_ = sub;
        sub->parent(menu_);
    }

    std::string_view MenuItem::label() const {
        return label_;
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

    void MenuItem::menu(boost::weak_ptr<Menu> m) {
        menu_ = m;
    }

    boost::weak_ptr<Menu> MenuItem::menu() {
        return menu_;
    }

    menu::ItemType MenuItem::type() const {
        return type_;
    }

};  // namespace v3d::ui::component
