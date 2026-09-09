/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "GameMenu.h"

#include <string>
#include <string_view>

namespace v3d::ui::shell {

const char* const GameMenu::defaultContainer = "game-menu";
const char* const GameMenu::defaultMenu = "main-menu";

/**
 **/
GameMenu::GameMenu(const boost::shared_ptr<Engine>& engine, const Suspend& suspend,
    const std::string& container, const std::string& menu) :
    engine_(engine),
    suspend_(suspend),
    containerName_(container),
    menuName_(menu) {
}

/**
 **/
boost::shared_ptr<Container> GameMenu::container() const {
    if (!engine_) {
        return boost::shared_ptr<Container>();
    }
    return engine_->container(containerName_);
}

/**
 **/
boost::shared_ptr<component::Menu> GameMenu::menu() const {
    boost::shared_ptr<Container> holder = container();
    if (!holder) {
        return boost::shared_ptr<component::Menu>();
    }
    return boost::dynamic_pointer_cast<component::Menu>(holder->get(menuName_));
}

/**
 **/
bool GameMenu::visible() const {
    boost::shared_ptr<Container> holder = container();
    return holder && holder->visible();
}

/**
 **/
void GameMenu::toggle() {
    boost::shared_ptr<Container> holder = container();
    if (!holder) {
        return;
    }

    if (!holder->visible()) {
        if (suspend_) {
            suspend_(true);
        }
        holder->visible(true);
        return;
    }

    boost::shared_ptr<component::Menu> active = menu();
    if (active && active->up()) {
        return;
    }
    holder->visible(false);
    if (suspend_) {
        suspend_(false);
    }
}

/**
 **/
bool GameMenu::capturing() const {
    if (!visible()) {
        return false;
    }
    boost::shared_ptr<component::Menu> active = menu();
    return active && active->capturing();
}

/**
 **/
bool GameMenu::capture(const v3d::event::EventData& value) {
    if (!visible()) {
        return false;
    }
    boost::shared_ptr<component::Menu> active = menu();
    if (!active) {
        return false;
    }
    return active->capture(value);
}

/**
 **/
bool GameMenu::navigate(const std::string_view& command) {
    if (!visible()) {
        return false;
    }
    boost::shared_ptr<component::Menu> active = menu();
    if (!active) {
        return false;
    }

    if (command == "menuPrevious") {
        active->previous();
    } else if (command == "menuNext") {
        active->next();
    } else if (command == "selectMenu") {
        active->activate();
    } else {
        return false;
    }
    return true;
}

};  // namespace v3d::ui::shell
