/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "Commands.h"

#include "Event.h"
#include "State.h"

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace v3d::event {

/**
 **/
bool Commands::add(const std::string& name, const Handler& handler) {
    if (name.empty() || !handler) {
        return false;
    }
    return handlers_.emplace(name, handler).second;
}

/**
 **/
bool Commands::addPress(const std::string& name, const PressHandler& handler) {
    if (!handler) {
        return false;
    }
    return add(name, [handler](const Event& event) {
        // a held key's repeats would run the action again at the repeat rate
        if (event.state() != State::Released && !event.repeat()) {
            handler();
        }
    });
}

/**
 **/
bool Commands::addHeld(const std::string& name) {
    return add(name, [](const Event&) { });
}

/**
 **/
bool Commands::remove(const std::string& name) {
    return handlers_.erase(name) > 0;
}

/**
 **/
bool Commands::has(const std::string& name) const {
    return handlers_.contains(name);
}

/**
 **/
bool Commands::invoke(const Event& event) const {
    const std::map<std::string, Handler>::const_iterator entry = handlers_.find(event.str());
    if (entry == handlers_.end()) {
        return false;
    }
    entry->second(event);
    return true;
}

/**
 **/
std::vector<std::string> Commands::names() const {
    std::vector<std::string> names;
    names.reserve(handlers_.size());
    for (const std::pair<const std::string, Handler>& entry : handlers_) {
        names.push_back(entry.first);
    }
    return names;
}

/**
 **/
std::vector<std::string> Commands::unhandled(const std::vector<std::string>& commands) const {
    std::vector<std::string> missing;
    for (const std::string& command : commands) {
        if (!has(command)) {
            missing.push_back(command);
        }
    }
    return missing;
}

/**
 **/
std::size_t Commands::size() const noexcept {
    return handlers_.size();
}

};  // namespace v3d::event
