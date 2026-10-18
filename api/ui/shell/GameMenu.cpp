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
const char* const GameMenu::toggleCommand = "showGameMenu";

/**
 **/
GameMenu::GameMenu(const boost::shared_ptr<Engine>& engine, const Suspend& suspend, const Names& names) :
    engine_(engine),
    suspend_(suspend),
    containerName_(names.container),
    menuName_(names.menu) {
}

/**
 **/
GameMenu::GameMenu(const boost::shared_ptr<Engine>& engine, const Suspend& suspend) :
    GameMenu(engine, suspend, Names()) {
}

/**
 **/
void GameMenu::declare(v3d::event::Commands& commands, const boost::shared_ptr<GameMenu>& menu) {
    // a press handler ignores a held key's repeats, which would otherwise open and close the
    // menu at the repeat rate
    commands.addPress(std::string("ui::") + toggleCommand, [&menu]() {
        if (menu) {
            menu->toggle();
        }
    });
    for (const char* const name : { "menuPrevious", "menuNext", "selectMenu" }) {
        commands.addPress(std::string("ui::") + name, [&menu, name]() {
            if (menu) {
                menu->navigate(name);
            }
        });
    }
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
boost::shared_ptr<component::menu::Menu> GameMenu::menu() const {
    const boost::shared_ptr<Container> holder = container();
    if (!holder) {
        return boost::shared_ptr<component::menu::Menu>();
    }
    return boost::dynamic_pointer_cast<component::menu::Menu>(holder->get(menuName_));
}

/**
 **/
bool GameMenu::visible() const {
    const boost::shared_ptr<Container> holder = container();
    return holder && holder->visible();
}

/**
 **/
void GameMenu::toggle() {
    const boost::shared_ptr<Container> holder = container();
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

    const boost::shared_ptr<component::menu::Menu> active = menu();
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
    const boost::shared_ptr<component::menu::Menu> active = menu();
    return active && active->capturing();
}

/**
 **/
bool GameMenu::capture(const v3d::event::EventData& value) {
    if (!visible()) {
        return false;
    }
    const boost::shared_ptr<component::menu::Menu> active = menu();
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
    const boost::shared_ptr<component::menu::Menu> active = menu();
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
