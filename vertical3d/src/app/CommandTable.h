/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Commands.h>
#include <vertical3d/src/script/Arguments.h>
#include <vertical3d/src/script/Journal.h>

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor::app {

/**
 * Where each part of the editor registers its commands. A command registered through press(),
 * hold() or script() reads the editor's state back into the ui after it runs, so a check item
 * shows what the editor holds rather than remembering its own state. A name registered a second
 * time is refused, and kept so that it can be reported once the log exists.
 *
 * A command registered through script() can also be run by a script. However it is run, the
 * journal lists it as the line that runs it again, with every argument it ran with.
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
     * A command a script can run. It is given what a script passed, and fills every parameter
     * left out from the editor's state, through the Arguments it is given. A menu or a key runs
     * it on a press with nothing passed.
     **/
    using Scripted = std::function<void(script::Arguments* arguments)>;

    /**
     * Register a command a script can run, with the parameters it takes.
     **/
    void script(const std::string& name, const std::vector<script::Parameter>& parameters, const Scripted& handler);

    /**
     * Register a command a script can run that takes nothing.
     **/
    void script(const std::string& name, const v3d::event::Commands::PressHandler& handler);

    /**
     * Run a command as a script does.
     * @param given the arguments, by parameter name
     * @return why it did not run: no command a script can run has the name, or an argument does
     *         not suit it. Nothing when it ran, whether or not it did what it was asked
     **/
    std::optional<std::string> run(const std::string& name, const boost::json::object& given);

    /**
     * @return each command a script can run, by name, with the parameters it takes
     **/
    std::map<std::string, std::vector<script::Parameter>> scripted() const;

    /**
     * @return what was run and what was said
     **/
    const boost::shared_ptr<script::Journal>& journal() const noexcept;

    /**
     * @return the names registered a second time, whose handlers never run
     **/
    const std::vector<std::string>& refused() const noexcept;

 private:
    /**
     * A command a script can run: what it takes, and its handler.
     **/
    struct Script final {
        std::vector<script::Parameter> parameters;
        Scripted handler;
    };

    /**
     * Run a command a script can run with some arguments, and list it in the journal.
     **/
    void execute(const std::string& name, const Script& command, script::Arguments arguments);

    v3d::event::Commands* commands_;
    std::function<void()> after_;
    std::vector<std::string> refused_;
    std::map<std::string, Script> scripts_;
    boost::shared_ptr<script::Journal> journal_;
};

};  // namespace v3d::editor::app
