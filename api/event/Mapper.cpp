/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Mapper.h"

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace v3d::event {

Mapper::Mapper(const std::string& name) : name_(name) {
}

std::string_view Mapper::name() const {
    return name_;
}

void Mapper::map(const Event& source, const Event& destination) {
    mappings_.insert(std::make_pair(source, destination));
}

std::vector<Event> Mapper::destinations(const Event& source) const {
    std::vector<Event> found;
    // events order by identity and then by state, so every binding on this source sits in
    // one contiguous run between the lowest and highest state
    Event first(source);
    first.state(State::Any);
    Event last(source);
    last.state(State::Released);
    auto begin = mappings_.lower_bound(first);
    auto end = mappings_.upper_bound(last);
    for (auto it = begin; it != end; ++it) {
        const State bound = it->first.state();
        if (bound == State::Any || bound == source.state()) {
            found.push_back(it->second);
        }
    }
    return found;
}

std::vector<Event> Mapper::sources(std::string_view destination) const {
    std::vector<Event> found;
    // the map is keyed by source, so finding a destination's sources scans all of it. That
    // is a handful of bindings, looked up a few times a step
    for (const std::pair<const Event, Event>& mapping : mappings_) {
        if (mapping.second.str() == destination) {
            found.push_back(mapping.first);
        }
    }
    return found;
}

/**
 **/
std::vector<std::string> Mapper::destinations() const {
    std::set<std::string> names;
    for (const std::pair<const Event, Event>& mapping : mappings_) {
        names.insert(mapping.second.str());
    }
    return std::vector<std::string>(names.begin(), names.end());
}

};  // namespace v3d::event
