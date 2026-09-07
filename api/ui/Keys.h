/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string_view>

#include <boost/shared_ptr.hpp>

#include <entt/entt.hpp>

namespace v3d::ui {

class Component;
class Engine;

/**
 * Turns a key into an edit on whatever has the focus, per ADR-0040.
 *
 * The keyboard's ui::Cursor, and the same shape: it is handed what the app's input engine
 * saw, it answers whether the ui took it, and it names the operation while the component
 * carries it out. Nothing here reaches for a component by walking the tree - the focus is
 * the ui's, given by a press, so a key goes to one place or to nowhere.
 *
 * Two kinds of input, because a key is not a character. A key names an operation - a
 * backspace, a caret move, a return that sends the command - and comes from the key names
 * api/input gives. A character is what the platform composed, and arrives whole: shift
 * has already been applied to it, a dead key and the one after it are one character, and
 * an input method's several keys are however many characters it decided on.
 *
 * A ui with nothing focused takes neither, which is what leaves a game's movement keys
 * working until the moment something is clicked into.
 **/
class Keys final {
 public:
    /**
     * @param ui where the focus lives
     * @param dispatcher where a focused component's event is sent
     **/
    Keys(const boost::shared_ptr<Engine>& ui, const boost::shared_ptr<entt::dispatcher>& dispatcher);

    /**
     * A key went down.
     *
     * @param key the name api/input gives it - "backspace", "arrow_left", "return"
     * @return whether the ui took it, which is what stops it reaching the app's bindings
     **/
    bool press(std::string_view key);

    /**
     * Characters the platform composed.
     *
     * @param utf8 what to put in at the caret
     * @return whether the ui took it
     **/
    bool text(std::string_view utf8);

 private:
    /**
     * Act on a key that reached a focused component: move the caret, take a character
     * out, or send whatever command the component carries.
     *
     * @return whether the component had anything to do with the key
     **/
    bool act(const boost::shared_ptr<Component>& component, std::string_view key);

    boost::shared_ptr<Engine> ui_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
};

};  // namespace v3d::ui
