/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CommandTable.h"

#include <api/event/Commands.h>
#include <api/event/Event.h>
#include <vertical3d/src/script/Arguments.h>
#include <vertical3d/src/script/Journal.h>
#include <vertical3d/src/script/Text.h>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor::app {

/**
 **/
CommandTable::CommandTable(v3d::event::Commands* commands, std::function<void()> after) :
    commands_(commands),
    after_(std::move(after)),
    journal_(boost::make_shared<script::Journal>()) {
}

/**
 **/
void CommandTable::press(const std::string& name, const v3d::event::Commands::PressHandler& handler) {
    if (!commands_->addPress(name, [this, handler]() {
        handler();
        after_();
    })) {
        refused_.push_back(name);
    }
}

/**
 **/
void CommandTable::hold(const std::string& name, const v3d::event::Commands::Handler& handler) {
    if (!commands_->add(name, [this, handler](const v3d::event::Event& event) {
        handler(event);
        after_();
    })) {
        refused_.push_back(name);
    }
}

/**
 **/
void CommandTable::add(const std::string& name, const v3d::event::Commands::Handler& handler) {
    if (!commands_->add(name, handler)) {
        refused_.push_back(name);
    }
}

/**
 **/
void CommandTable::script(const std::string& name, const std::vector<script::Parameter>& parameters, const Scripted& handler) {
    const auto run = [this, name]() { execute(name, scripts_.at(name), script::Arguments()); };
    if (scripts_.contains(name) || !commands_->addPress(name, run)) {
        refused_.push_back(name);
        return;
    }
    scripts_.emplace(name, Script{ .parameters = parameters, .handler = handler });
}

/**
 **/
void CommandTable::script(const std::string& name, const v3d::event::Commands::PressHandler& handler) {
    script(name, {}, [handler](script::Arguments*) { handler(); });
}

/**
 **/
std::optional<std::string> CommandTable::run(const std::string& name, const boost::json::object& given) {
    const auto found = scripts_.find(name);
    if (found == scripts_.end()) {
        return "no command a script can run is called " + script::path(name);
    }
    if (const std::optional<std::string> problem = script::Arguments::check(given, found->second.parameters)) {
        return script::path(name) + ": " + *problem;
    }
    execute(name, found->second, script::Arguments(given));
    return std::nullopt;
}

/**
 **/
std::map<std::string, std::vector<script::Parameter>> CommandTable::scripted() const {
    std::map<std::string, std::vector<script::Parameter>> each;
    for (const auto& [name, command] : scripts_) {
        each.emplace(name, command.parameters);
    }
    return each;
}

/**
 **/
const boost::shared_ptr<script::Journal>& CommandTable::journal() const noexcept {
    return journal_;
}

/**
 **/
void CommandTable::execute(const std::string& name, const Script& command, script::Arguments arguments) {
    // the line goes in before the command runs, so what it says follows it, and is written
    // again with the arguments it filled once it has run
    const std::uint64_t entry = journal_->add(script::Journal::Kind::Command, script::line(name, command.parameters, arguments.values()));
    command.handler(&arguments);
    journal_->rewrite(entry, script::line(name, command.parameters, arguments.values()));
    after_();
}

/**
 **/
const std::vector<std::string>& CommandTable::refused() const noexcept {
    return refused_;
}

};  // namespace v3d::editor::app
