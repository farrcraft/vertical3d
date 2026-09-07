/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "CommandDirectory.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace v3d::editor {

/**
 **/
bool CommandDirectory::add(const std::string& name, const Handler& handler) {
    if (name.empty() || !handler) {
        return false;
    }
    return handlers_.emplace(name, handler).second;
}

/**
 **/
bool CommandDirectory::addPress(const std::string& name, const PressHandler& handler) {
    if (!handler) {
        return false;
    }
    return add(name, [handler](const v3d::event::Event& event) {
        if (event.state() != v3d::event::State::Released) {
            handler();
        }
    });
}

/**
 **/
bool CommandDirectory::has(const std::string& name) const {
    return handlers_.contains(name);
}

/**
 **/
bool CommandDirectory::invoke(const v3d::event::Event& event) const {
    const std::map<std::string, Handler>::const_iterator entry = handlers_.find(event.str());
    if (entry == handlers_.end()) {
        return false;
    }
    entry->second(event);
    return true;
}

/**
 **/
std::vector<std::string> CommandDirectory::names() const {
    std::vector<std::string> names;
    names.reserve(handlers_.size());
    for (const std::pair<const std::string, Handler>& entry : handlers_) {
        names.push_back(entry.first);
    }
    return names;
}

/**
 **/
std::size_t CommandDirectory::size() const noexcept {
    return handlers_.size();
}

};  // namespace v3d::editor
