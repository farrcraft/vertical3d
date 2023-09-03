/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "MenuItem.h"
#include "Menu.h"

namespace v3d::ui {

    MenuItem::MenuItem(menu::ItemType type, const std::string& label,
        const std::string& cmd, const std::string& scope) :
        Component(component::Type::MENU_ITEM),
        label_(label), type_(type), command_(cmd), scope_(scope) {
    }

    void MenuItem::draw() const {
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

    std::string_view MenuItem::command() const {
        return command_;
    }

    void MenuItem::menu(boost::weak_ptr<Menu> m) {
        menu_ = m;
    }

    boost::weak_ptr<Menu> MenuItem::menu() {
        return menu_;
    }

    std::string_view MenuItem::scope() const {
        return scope_;
    }

    menu::ItemType MenuItem::type() const {
        return type_;
    }

};  // namespace v3d::ui
