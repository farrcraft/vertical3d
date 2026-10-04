/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Mapper.h"

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
        State bound = it->first.state();
        if (bound == State::Any || bound == source.state()) {
            found.push_back(it->second);
        }
    }
    return found;
}

std::vector<Event> Mapper::sources(std::string_view destination) const {
    std::vector<Event> found;
    // the map is keyed by source, so a destination is a walk over all of it - which is a
    // handful of bindings, asked a handful of times a step
    for (const std::pair<const Event, Event>& mapping : mappings_) {
        if (mapping.second.str() == destination) {
            found.push_back(mapping.first);
        }
    }
    return found;
}

};  // namespace v3d::event
