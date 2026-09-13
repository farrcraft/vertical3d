/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>

#include <functional>
#include <string>
#include <string_view>

#include <boost/shared_ptr.hpp>

#include <entt/entt.hpp>

namespace v3d::ui {

class Component;
class Engine;

namespace component {
class Scrollbar;
class SelectList;
class TabBar;
class TextBox;
};  // namespace component

};  // namespace v3d::ui

namespace v3d::ui::input {

/**
 * Turns a key into an edit on whatever has the focus, per ADR-0040.
 *
 * The keyboard's ui::Cursor, and the same shape: it is handed what the app's input engine
 * saw, it answers whether the ui took it, and it names the operation while the component
 * carries it out. Nothing here reaches for a component by walking the tree - the focus is
 * the ui's, so a key goes to one place or to nowhere.
 *
 * Two kinds of input, because a key is not a character. A key names an operation - a
 * backspace, a caret move, a return that sends the command - and comes from the key names
 * api/input gives. A character is what the platform composed, and arrives whole: shift
 * has already been applied to it, a dead key and the one after it are one character, and
 * an input method's several keys are however many characters it decided on.
 *
 * Every control is driven, not only a text box. Return and space activate whatever holds
 * the focus, sending the command a click sends because both ask ui::command() for it; the
 * arrows step through a list's rows and a tab bar's pages and move a scrollbar by a line.
 * Only a text box takes the keys that compose text - a letter reaching a focused button goes
 * on to the app's bindings, because a button is not something a player is typing into.
 *
 * A box also takes the four chords an editor is expected to answer - cut, copy, paste and
 * select all - over the selection ADR-0057 gave it. The clipboard behind them is the app's,
 * for the reason the text measuring is.
 *
 * A ui with nothing focused takes neither kind, which is what leaves a game's movement keys
 * working until something is clicked into or Engine::focusFirst() starts a screen off.
 **/
class Keys final {
 public:
    /**
     * The platform's clipboard, which this library cannot reach for itself without taking
     * SDL with it - the same kind of seam text measuring is, per ADR-0019 and ADR-0057.
     *
     * A router given neither call still edits everything else. A cut with nowhere to hand
     * the run does not take it out, because a cut that loses the text is worse than one
     * that did not happen, and a paste with nothing to read puts nothing in.
     **/
    struct Clipboard final {
        std::function<std::string()> read;
        std::function<void(std::string_view)> write;
    };

    /**
     * @param ui where the focus lives
     * @param dispatcher where a focused component's event is sent
     * @param clipboard where a cut and a copy hand the selected run, and where a paste
     *        reads one from
     **/
    Keys(const boost::shared_ptr<Engine>& ui, const boost::shared_ptr<entt::dispatcher>& dispatcher,
        const Clipboard& clipboard = Clipboard());

    /**
     * A key went down.
     *
     * @param key the name api/input gives it - "backspace", "arrow_left", "return", "tab"
     * @param shifted whether a shift key is held. A key name carries no modifier and this
     *        library cannot ask api/input for one without taking SDL with it, so the app
     *        that saw the key says. Shift and tab is the focus moving backwards, and shift
     *        and a caret key selects the run it travelled
     * @param controlled whether a control key is held, which names the four chords a text
     *        box answers - cut, copy, paste and select all. Every other chord goes on to
     *        the app, so a ctrl-s still saves while somebody is typing
     * @return whether the ui took it, which is what stops it reaching the app's bindings
     **/
    bool press(std::string_view key, bool shifted = false, bool controlled = false);

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
     * out, step through what the component holds, or send whatever command it carries.
     *
     * @return whether the component had anything to do with the key
     **/
    bool act(const boost::shared_ptr<Component>& component, std::string_view key,
        bool shifted, bool controlled);

    /**
     * A key that reached a text box: the caret moves, a character goes, or a return sends.
     *
     * The one component that takes every key that composes text, because a box is the one
     * place a letter is being typed rather than played.
     **/
    bool edit(const boost::shared_ptr<component::TextBox>& box, std::string_view key,
        bool shifted, bool controlled);

    /**
     * Hand the selected run to the clipboard, and take it out of the box for a cut.
     *
     * Neither does anything without a selection, because a copy of nothing would leave the
     * clipboard holding an empty string in place of whatever was in it.
     **/
    void copySelection(const boost::shared_ptr<component::TextBox>& box) const;
    void cutSelection(const boost::shared_ptr<component::TextBox>& box) const;

    /**
     * Put whatever the clipboard holds in over the selection, which is what TextBox::insert
     * already does with a run of characters the platform composed.
     **/
    void paste(const boost::shared_ptr<component::TextBox>& box) const;

    /**
     * A key that reached a select list: the arrows step through the rows and send the
     * command, the way clicking a row does.
     **/
    bool choose(const boost::shared_ptr<component::SelectList>& list, std::string_view key);

    /**
     * A key that reached a tab bar: the arrows change which page is up. A bar carries no
     * command, so nothing is sent - which is what a click on a tab does too.
     **/
    static bool turn(const boost::shared_ptr<component::TabBar>& bar, std::string_view key);

    /**
     * A key that reached a scrollbar: the arrows move it by a line, page up and page down
     * by what the page shows, and home and end to the ends of the content.
     *
     * The bar is the arithmetic and not the input, so what a line and a page come to is
     * its to say and this only names which of them a key asked for. Which arrows read as
     * "along" is the bar's direction, the way it is for a list and for a tab bar.
     *
     * A bar with nothing to scroll takes no key, so an arrow reaching one that shows all
     * of its content goes on to the app's bindings rather than being swallowed by a
     * control that could not have moved.
     **/
    static bool nudge(const boost::shared_ptr<component::Scrollbar>& bar, std::string_view key);

    /**
     * Send whatever command activating a component sends, per ui::command(). A component
     * carrying none is left alone rather than being an error.
     **/
    void send(const boost::shared_ptr<Component>& component) const;

    /**
     * Send one event. An event with no context is not dispatchable and is dropped, which
     * is what a component nobody gave a command to carries.
     **/
    void send(const v3d::event::Event& event) const;

    boost::shared_ptr<Engine> ui_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    Clipboard clipboard_;
};

};  // namespace v3d::ui::input
