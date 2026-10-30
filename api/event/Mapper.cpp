/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Mapper.h"

#include "Chord.h"
#include "Event.h"
#include "State.h"

#include <algorithm>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace v3d::event {

Mapper::Mapper(const std::string& name) : name_(name) {
}

std::string_view Mapper::name() const {
    return name_;
}

void Mapper::map(const Event& source, const Event& destination) {
    const Chord split = chord(source.name());
    Event key(split.key, source.context());
    key.type(source.type());
    key.state(source.state());
    mappings_.insert(std::make_pair(key, Binding{.destination = destination, .modifiers = split.modifiers, .source = source}));
}

std::vector<Event> Mapper::destinations(const Event& source, const Modifiers& held) const {
    std::vector<const Binding*> matched;
    // events order by identity and then by state, so every binding on this source sits in
    // one contiguous run between the lowest and highest state
    Event first(source);
    first.state(State::Any);
    Event last(source);
    last.state(State::Released);
    auto begin = mappings_.lower_bound(first);
    auto end = mappings_.upper_bound(last);
    int most = 0;
    for (auto it = begin; it != end; ++it) {
        const State bound = it->first.state();
        const Binding& binding = it->second;
        if ((bound == State::Any || bound == source.state()) && binding.modifiers.within(held)) {
            matched.push_back(&binding);
            most = std::max(most, binding.modifiers.count());
        }
    }

    // a chord claims its key from every binding that names fewer modifiers
    std::vector<Event> found;
    for (const Binding* binding : matched) {
        if (binding->modifiers.count() == most) {
            found.push_back(binding->destination);
        }
    }
    return found;
}

std::vector<Event> Mapper::sources(std::string_view destination) const {
    std::vector<Event> found;
    // the map is keyed by source, so this scans every binding. A binding document holds few
    // enough for that to be cheap
    for (const std::pair<const Event, Binding>& mapping : mappings_) {
        if (mapping.second.destination.str() == destination) {
            found.push_back(mapping.second.source);
        }
    }
    return found;
}

/**
 **/
std::vector<std::string> Mapper::destinations() const {
    std::set<std::string> names;
    for (const std::pair<const Event, Binding>& mapping : mappings_) {
        names.insert(mapping.second.destination.str());
    }
    return std::vector<std::string>(names.begin(), names.end());
}

};  // namespace v3d::event
