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
 * What an app can be asked to do, by name.
 *
 * A command is identified by a "context::name" string, the value `Event::str()` returns, and
 * the registry maps that string to a handler. A key binding and a menu item that carry the same
 * event therefore reach the same handler. The registry does not record where an invocation came
 * from.
 *
 * Every command an app handles is registered, including one it reads by polling whether its
 * key is held rather than by handling the event. A binding whose command is not registered is
 * one nothing handles, and unhandled() lists those.
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
     * A handler for a command that only ever happens on the press.
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
     * Register a handler that runs on the press only. A release of the same binding is
     * still handled - it just does nothing - so the command does not report as unknown. A held
     * key's repeat is handled the same way, and holding the key runs the action once.
     * @param name the command's "context::name"
     * @param handler what to run
     * @return false when the name is already registered, leaving the first handler in place
     **/
    bool addPress(const std::string& name, const PressHandler& handler);

    /**
     * Register a command that is read by asking whether its key is held, not by handling its
     * event. Its event is handled and does nothing.
     * @param name the command's "context::name"
     * @return false when the name is already registered
     **/
    bool addHeld(const std::string& name);

    /**
     * Forget a command, so its handler can no longer run.
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
