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

namespace v3d::editor {

/**
 * What the editor can be asked to do, by name.
 *
 * A command is identified by a "context::name" string, the value `event::Event::str()`
 * returns, and the directory maps that string to a handler. A key binding and a menu item
 * that carry the same event therefore reach the same handler. The directory does not
 * record where an invocation came from.
 *
 * A name with no handler is not an error here: `invoke` returns false and the caller
 * decides what that means.
 *
 * The directory and `Tool` belong to the editor, not to the api. Only the editor invokes
 * one command from several places, and only the editor has tools that hold a gesture open.
 **/
class CommandDirectory final {
 public:
    /**
     * A handler sees the whole event, so that a command bound to both edges of a key
     * can tell a press from a release.
     **/
    using Handler = std::function<void(const v3d::event::Event&)>;

    /**
     * A handler for a command that only ever happens on the press.
     **/
    using PressHandler = std::function<void()>;

    CommandDirectory() = default;

    /**
     * Register a handler for every state the command arrives in.
     * @param name the command's "context::name"
     * @param handler what to run
     * @return false when the name is already registered, leaving the first handler in place
     **/
    bool add(const std::string& name, const Handler& handler);

    /**
     * Register a handler that runs on the press only. A release of the same binding is
     * still handled - it just does nothing - so the command does not report as unknown.
     * @param name the command's "context::name"
     * @param handler what to run
     * @return false when the name is already registered, leaving the first handler in place
     **/
    bool addPress(const std::string& name, const PressHandler& handler);

    /**
     **/
    bool has(const std::string& name) const;

    /**
     * Run the handler the event names.
     * @param event the event, whose identity is the command name
     * @return false when no handler is registered for it
     **/
    bool invoke(const v3d::event::Event& event) const;

    /**
     * Every registered command, in name order. A menu definition can be checked against
     * this list.
     **/
    std::vector<std::string> names() const;

    /**
     **/
    std::size_t size() const noexcept;

 private:
    std::map<std::string, Handler> handlers_;
};

};  // namespace v3d::editor
