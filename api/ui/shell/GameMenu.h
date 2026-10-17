/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Commands.h>
#include <api/event/Event.h>
#include <api/ui/Container.h>
#include <api/ui/Engine.h>
#include <api/ui/component/menu/Menu.h>

#include <functional>
#include <string>
#include <string_view>

#include <boost/shared_ptr.hpp>

namespace v3d::ui::shell {

/**
 * The menu a game puts up over itself, and the commands that drive it.
 *
 * The container is what is shown and hidden, not the menu: a component is visible from the
 * moment it is built, so asking the menu whether it is up gives the wrong answer.
 *
 * Both names come from the app's ui config, so a container this cannot find leaves every
 * call here doing nothing rather than failing - a game whose config names no menu still
 * runs, without one.
 *
 * declare() registers its commands. "ui::showGameMenu" toggles it and the navigation commands
 * navigate() names drive it, so an app's own handlers are only for the commands that are its
 * game's.
 **/
class GameMenu {
 public:
    /**
     * What the menu going up and coming down does to the game under it - pausing it, and
     * whatever else the app cannot be played without while a menu is over it.
     *
     * @param suspended true as the menu opens and false as it closes
     **/
    typedef std::function<void(bool suspended)> Suspend;

    /**
     * The container and the menu an app's ui config is expected to name.
     **/
    static const char* const defaultContainer;
    static const char* const defaultMenu;

    /** The container that is shown and hidden, and the menu inside it that navigation drives. **/
    struct Names {
        std::string container = defaultContainer;
        std::string menu = defaultMenu;
    };

    /**
     * @param engine the ui the container is looked up in
     * @param suspend what to do to the game as the menu opens and closes
     * @param names the container that is shown and hidden, and the menu inside it that
     *        navigation drives
     **/
    GameMenu(const boost::shared_ptr<Engine>& engine, const Suspend& suspend, const Names& names = Names());

    /**
     * Register the menu's commands: "ui::showGameMenu" toggles it, and "ui::menuPrevious",
     * "ui::menuNext" and "ui::selectMenu" navigate it. A held key's repeats toggle nothing.
     *
     * An app calls this from its constructor, before the menu exists, and passes the member it
     * will hold the menu in. Each command does nothing while that member is empty.
     *
     * @param commands the app's commands
     * @param menu the member the app holds its menu in, which must outlive the commands
     **/
    static void declare(v3d::event::Commands& commands, const boost::shared_ptr<GameMenu>& menu);

    /**
     * The command a game's escape key and its menu's back item send, which toggles the menu.
     **/
    static const char* const toggleCommand;

    /**
     * @return whether the menu is up, which suspends the game under it
     **/
    bool visible() const;

    /**
     * Put the menu up, or take one step back out of it.
     *
     * Going back up out of a submenu leaves the menu open - it is only closing the top
     * level that resumes the game.
     **/
    void toggle();

    /**
     * Act on one of the navigation commands an app's bindings send while the menu is up -
     * "menuPrevious", "menuNext" and "selectMenu", which the mappings in an app's
     * data directory name.
     *
     * @return whether the command was one of them and was acted on, which a command
     *         arriving while the menu is down is not
     **/
    bool navigate(const std::string_view& command);

    /**
     * Whether an input item is capturing, so that the app can send what it captures here
     * rather than acting on it itself.
     **/
    bool capturing() const;

    /**
     * Give the capturing item its value - a key name for a key input, per the table in
     * api/input/Keyboard.cpp.
     *
     * An app feeds this from its key events while capturing() rather than binding them.
     * That is how a rebinding screen works: the key pressed becomes the new binding instead
     * of doing what it is bound to.
     *
     * @return whether a capture took it, which is false when the menu is down or when
     *         nothing is capturing
     **/
    bool capture(const v3d::event::EventData& value);

 private:
    boost::shared_ptr<Container> container() const;
    boost::shared_ptr<component::menu::Menu> menu() const;

    boost::shared_ptr<Engine> engine_;
    Suspend suspend_;
    std::string containerName_;
    std::string menuName_;
};

};  // namespace v3d::ui::shell
