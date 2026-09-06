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

};  // namespace v3d::event
