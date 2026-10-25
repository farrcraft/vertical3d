/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>
#include <api/ui/input/LineEdit.h>

#include <string>
#include <string_view>

#include <boost/shared_ptr.hpp>

#include <entt/entt.hpp>

namespace v3d::ui {

class Component;
class Engine;

namespace component {
class Scrollbar;
class Slider;
class SelectList;
class TabBar;
class TextBox;
};  // namespace component

};  // namespace v3d::ui

namespace v3d::ui::input {

/**
 * Turns a key into an edit on whatever has the focus.
 *
 * The keyboard counterpart of input::Cursor, with the same shape. It receives what the
 * app's input engine saw and returns whether the ui took it. It chooses the operation, and
 * the component carries it out. Nothing here searches the tree for a component. The engine
 * holds the focus, so a key goes to one component or to none.
 *
 * There are two kinds of input. A key names an operation, such as a backspace, a caret
 * move, or a return that sends the command. Its name comes from api/input. A character is
 * text the platform composed, and arrives whole. Shift has already been applied to it, a
 * dead key and the key after it make one character, and an input method sends however many
 * characters it produced.
 *
 * Every control can be driven from the keyboard, not only a text box. Return and space
 * activate the focused component, sending the same command as a click, because both get it
 * from input::command(). The arrows step through a list's rows and a tab bar's pages, and
 * move a scrollbar by a line. Only a text box takes the keys that compose text. A letter
 * reaching a focused button goes on to the app's bindings.
 *
 * A text box also takes the cut, copy, paste and select all chords, which act on its
 * selection. The app supplies the clipboard, as it supplies text measuring.
 *
 * A ui with nothing focused takes neither kind of input, so a game's movement keys keep
 * working until something is clicked into or Engine::focusFirst() is called.
 **/
class Keys final {
 public:
    /**
     * The platform's clipboard, which a text box's cut, copy and paste read and write.
     **/
    typedef input::Clipboard Clipboard;

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
     * @param shifted whether a shift key is held. A key name carries no modifier, and this
     *        library cannot ask api/input for one without depending on SDL, so the app
     *        passes it. Shift and tab moves the focus backwards, and shift and a caret key
     *        selects the text the caret moves over
     * @param controlled whether a control key is held. A text box takes ctrl with c, x, v
     *        and a, for copy, cut, paste and select all. Every other chord goes on to the
     *        app, so a ctrl-s still saves while somebody is typing
     * @param repeat whether this is the key repeating while it is held. A held return or
     *        space sends its command once: its repeats are taken and do nothing. Every other
     *        key acts on each repeat, so a held arrow keeps moving through a list
     * @return whether the ui took it, in which case it does not reach the app's bindings
     **/
    bool press(std::string_view key, bool shifted = false, bool controlled = false, bool repeat = false);

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
     * A key that reached a text box: the caret moves, a character is deleted, or a return
     * sends the command.
     *
     * A text box is the only component that takes every key that composes text, because
     * letters typed into it are text, not game controls.
     **/
    bool edit(const boost::shared_ptr<component::TextBox>& box, std::string_view key,
        bool shifted, bool controlled);

    /**
     * A key that reached a select list. The arrows, home and end step through the rows and
     * send the command, as clicking a row does. Return and space send it without moving.
     **/
    bool choose(const boost::shared_ptr<component::SelectList>& list, std::string_view key);

    /**
     * A key that reached a tab bar: the arrows, home and end change the chosen page. A tab
     * bar carries no command, so nothing is sent, as with a click on a tab.
     **/
    static bool turn(const boost::shared_ptr<component::TabBar>& bar, std::string_view key);

    /**
     * A key that reached a scrollbar: the arrows move it by a line, page up and page down
     * by what the page shows, and home and end to the ends of the content.
     *
     * The scrollbar decides how far a line and a page are. This only chooses which of them
     * a key moves by. A vertical bar takes the up and down arrows, and a horizontal bar the
     * left and right arrows.
     *
     * A bar with nothing to scroll takes no key, so an arrow reaching one that shows all
     * of its content goes on to the app's bindings.
     **/
    static bool nudge(const boost::shared_ptr<component::Scrollbar>& bar, std::string_view key);

    /**
     * Move a slider: the left and right arrows by a step, page up and down by a tenth of the
     * range, home and end to the ends. It sends its command when that changed the value.
     *
     * @return whether the key changed the value. A key that pushes past an end returns false
     **/
    bool slide(const boost::shared_ptr<component::Slider>& slider, std::string_view key);

    /**
     * Send the command input::command() returns for a component. A component carrying none
     * sends nothing, and that is not an error.
     **/
    void send(const boost::shared_ptr<Component>& component) const;

    /**
     * Send one event. An event with no context cannot be dispatched and is dropped. A
     * component with no command returns such an event.
     **/
    void send(const v3d::event::Event& event) const;

    boost::shared_ptr<Engine> ui_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    Clipboard clipboard_;
};

};  // namespace v3d::ui::input
