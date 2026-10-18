/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/event/Event.h>

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace v3d::event {

/**
 * A registry of the commands an app handles, each mapped by name to a handler.
 *
 * A command's name is the "context::name" string that `Event::str()` returns. A key binding
 * and a menu item that carry the same event therefore reach the same handler. The registry does
 * not record where an invocation came from.
 *
 * An app registers every command it handles. That includes a command the app reads by polling
 * whether its key is held, which addHeld() registers. unhandled() lists the bound commands that
 * are not registered.
 *
 * A name with no handler is not an error here: `invoke` returns false and the caller decides
 * what that means.
 **/
class Commands final {
 public:
    /**
     * A handler sees the whole event, so that a command bound to both edges of a key
     * can tell a press from a release.
     **/
    using Handler = std::function<void(const Event&)>;

    /**
     * A handler that runs only on a press.
     **/
    using PressHandler = std::function<void()>;

    Commands() = default;

    /**
     * Register a handler for every state the command arrives in.
     * @param name the command's "context::name"
     * @param handler what to run
     * @return false when the name is already registered, leaving the first handler in place
     **/
    bool add(const std::string& name, const Handler& handler);

    /**
     * Register a handler that runs on a press, or on an event with State::Any. A release and
     * a held key's repeat are still handled, so invoke() returns true, but the handler does not
     * run. Holding a key therefore runs the action once.
     * @param name the command's "context::name"
     * @param handler what to run
     * @return false when the name is already registered, leaving the first handler in place
     **/
    bool addPress(const std::string& name, const PressHandler& handler);

    /**
     * Register a command that the app reads by polling whether its key is held. Its event is
     * handled and does nothing.
     * @param name the command's "context::name"
     * @return false when the name is already registered
     **/
    bool addHeld(const std::string& name);

    /**
     * Remove a command and its handler.
     * @return false when it was not registered
     **/
    bool remove(const std::string& name);

    /**
     **/
    bool has(const std::string& name) const;

    /**
     * Run the handler the event names.
     * @param event the event, whose identity is the command name
     * @return false when no handler is registered for it
     **/
    bool invoke(const Event& event) const;

    /**
     * Every registered command, in name order. A menu definition can be checked against
     * this list.
     **/
    std::vector<std::string> names() const;

    /**
     * The commands in a list that are not registered, in the list's order.
     * @param commands the commands something sends, such as Bindings::commands()
     **/
    std::vector<std::string> unhandled(const std::vector<std::string>& commands) const;

    /**
     **/
    std::size_t size() const noexcept;

 private:
    std::map<std::string, Handler> handlers_;
};

};  // namespace v3d::event
