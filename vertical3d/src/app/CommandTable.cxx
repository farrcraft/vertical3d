/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CommandTable.h"

#include <api/event/Commands.h>
#include <api/event/Event.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace v3d::editor {

/**
 **/
CommandTable::CommandTable(v3d::event::Commands* commands, std::function<void()> after) :
    commands_(commands),
    after_(std::move(after)) {
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
const std::vector<std::string>& CommandTable::refused() const noexcept {
    return refused_;
}

};  // namespace v3d::editor
