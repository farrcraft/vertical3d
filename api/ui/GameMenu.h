/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <functional>
#include <string>
#include <string_view>

#include "Container.h"
#include "Engine.h"
#include "component/menu/Menu.h"

#include <boost/shared_ptr.hpp>

namespace v3d::ui {

/**
 * The menu a game puts up over itself, and the commands that drive it.
 *
 * The container is what is shown and hidden, not the menu: a component is visible from the
 * moment it is built, so asking the menu whether it is up gives the wrong answer.
 *
 * Both names come from the app's ui config, so a container this cannot find leaves every
 * call here doing nothing rather than failing - a game whose config names no menu still
 * runs, without one.
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

    /**
     * @param engine the ui the container is looked up in
     * @param suspend what to do to the game as the menu opens and closes
     * @param container the name of the container that is shown and hidden
     * @param menu the name of the menu inside it that navigation drives
     **/
    GameMenu(const boost::shared_ptr<Engine>& engine, const Suspend& suspend,
        const std::string& container = defaultContainer, const std::string& menu = defaultMenu);

    /**
     * @return whether the menu is up, which is what suspends the game under it
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
     * "menuPrevious", "menuNext" and "selectMenu", which is what the mappings in an app's
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
     * An app feeds this from its key events while capturing() rather than binding them,
     * which is what a rebinding screen is: the key that would normally do something is
     * instead the answer to what should do it.
     *
     * @return whether a capture took it, which is false when the menu is down or when
     *         nothing is capturing
     **/
    bool capture(const v3d::event::EventData& value);

 private:
    boost::shared_ptr<Container> container() const;
    boost::shared_ptr<component::Menu> menu() const;

    boost::shared_ptr<Engine> engine_;
    Suspend suspend_;
    std::string containerName_;
    std::string menuName_;
};

};  // namespace v3d::ui
