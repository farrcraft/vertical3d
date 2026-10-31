/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Commands.h>

#include <functional>
#include <string>
#include <vector>

namespace v3d::editor::app {

/**
 * Where each part of the editor registers its commands. A command registered through press()
 * or hold() reads the editor's state back into the ui after it runs, so a check item shows what
 * the editor holds rather than remembering its own state. A name registered a second time is
 * refused, and kept so that it can be reported once the log exists.
 **/
class CommandTable final {
 public:
    /**
     * @param commands the engine's commands, which the table adds to
     * @param after run after each command registered through press() or hold()
     **/
    CommandTable(v3d::event::Commands* commands, std::function<void()> after);

    /**
     * Register a handler that runs on a press only. A release and a held key's repeat do
     * nothing.
     **/
    void press(const std::string& name, const v3d::event::Commands::PressHandler& handler);

    /**
     * Register a handler that receives the whole event, so it can tell a press from a release.
     **/
    void hold(const std::string& name, const v3d::event::Commands::Handler& handler);

    /**
     * Register a handler that receives the whole event, and is not followed by the read back.
     * A command that changes the areas follows itself with what the areas need instead.
     **/
    void add(const std::string& name, const v3d::event::Commands::Handler& handler);

    /**
     * @return the names registered a second time, whose handlers never run
     **/
    const std::vector<std::string>& refused() const noexcept;

 private:
    v3d::event::Commands* commands_;
    std::function<void()> after_;
    std::vector<std::string> refused_;
};

};  // namespace v3d::editor::app
