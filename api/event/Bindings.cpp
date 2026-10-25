/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bindings.h"

#include <api/asset/Json.h>
#include <api/log/Logger.h>

#include "Engine.h"
#include "Event.h"
#include "State.h"
#include "Type.h"

#include <algorithm>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/json.hpp>
#include <boost/make_shared.hpp>

namespace v3d::event {

Bindings::Bindings(const boost::shared_ptr<Engine>& events, const boost::shared_ptr<v3d::log::Logger>& logger,
    const Known& known) :
    events_(events),
    logger_(logger),
    known_(known) {
}

bool Bindings::load(const boost::json::object& document) {
    const std::optional<boost::json::object> previous = document_;
    document_ = document;
    if (!build()) {
        document_ = previous;
        return false;
    }
    return true;
}

bool Bindings::rebind(const std::string& command, const std::string& name) {
    if (!document_) {
        return false;
    }
    // this override covers every source, so any earlier one of the command would be shadowed
    std::erase_if(overrides_, [&command](const Override& made) { return made.command == command; });
    overrides_.push_back(Override{.command = command, .source = std::string(), .name = name});
    return build();
}

bool Bindings::rebind(const std::string& command, const std::string& source, const std::string& name) {
    if (!document_ || source.empty() || !binds(command, source)) {
        return false;
    }
    std::erase_if(overrides_, [&command, &source](const Override& made) {
        return made.command == command && made.source == source;
    });
    overrides_.push_back(Override{.command = command, .source = source, .name = name});
    return build();
}

bool Bindings::reset(const std::string& command) {
    if (!document_) {
        return false;
    }
    std::erase_if(overrides_, [&command](const Override& made) { return made.command == command; });
    return build();
}

bool Bindings::reset() {
    if (!document_) {
        return false;
    }
    overrides_.clear();
    return build();
}

const std::vector<Bindings::Override>& Bindings::overrides() const noexcept {
    return overrides_;
}

bool Bindings::restore(const std::vector<Override>& overrides) {
    if (!document_) {
        return false;
    }
    overrides_.clear();
    for (const Override& made : overrides) {
        if (binds(made.command, made.source)) {
            overrides_.push_back(made);
        } else {
            logger_->get()->warn("A saved binding of {} names a source the bindings no longer have, so it is dropped", made.command);
        }
    }
    return build();
}

bool Bindings::binds(std::string_view command, std::string_view source) const {
    return std::ranges::any_of(declared_, [command, source](const std::pair<std::string, std::string>& binding) {
        return binding.first == command && (source.empty() || binding.second == source);
    });
}

std::vector<Event> Bindings::sources(std::string_view command) const {
    if (!mapper_) {
        return std::vector<Event>();
    }
    return mapper_->sources(command);
}

std::vector<std::string> Bindings::commands() const {
    if (!mapper_) {
        return std::vector<std::string>();
    }
    return mapper_->destinations();
}

std::vector<std::string> Bindings::bound(std::string_view name) const {
    std::vector<std::string> found;
    for (const std::string& command : commands()) {
        const std::vector<Event> sources = mapper_->sources(command);
        const bool fires = std::ranges::any_of(sources, [name](const Event& source) { return source.name() == name; });
        if (fires) {
            // commands() is in name order and holds each once, so found is too
            found.push_back(command);
        }
    }
    return found;
}

bool Bindings::build() {
    // read every member through the checked reads in api/asset/Json.h. Each returns nothing for
    // a member that is missing or of the wrong type. A malformed document returns false here,
    // and never throws.
    const boost::json::object& doc = *document_;
    const boost::json::array* mappings = v3d::asset::readArray(doc, "mappings");
    if (mappings == nullptr) {
        logger_->get()->error("Missing mappings in config");
        return false;
    }

    const boost::shared_ptr<Mapper> mapper = boost::make_shared<Mapper>("global");
    std::vector<std::pair<std::string, std::string>> declared;
    std::vector<std::pair<Event, Event>> installed;
    for (const boost::json::value& item : *mappings) {
        if (!item.is_object()) {
            logger_->get()->error("Unrecognized mapping");
            return false;
        }
        const boost::json::object& mapping = item.as_object();
        Event source;
        Event destination;
        if (!readSource(mapping, &source) || !readDestination(mapping, &destination)) {
            return false;
        }
        const std::string command = destination.str();
        declared.emplace_back(command, std::string(source.name()));
        source = rebound(source, command);
        if (known_ && !known_(source)) {
            logger_->get()->warn("{} is bound to {}, which no device sends, so it never fires", command, source.str());
        }
        install(&installed, source, destination);
    }
    for (const std::pair<Event, Event>& mapping : installed) {
        mapper->map(mapping.first, mapping.second);
    }
    // addMapper stores mappers by name, so this replaces the previous "global" mapper
    events_->addMapper(mapper);
    mapper_ = mapper;
    declared_ = std::move(declared);
    return true;
}

void Bindings::install(std::vector<std::pair<Event, Event>>* installed, const Event& source, const Event& destination) {
    // two keys of one command rebound to the same key can leave two mappings that a press
    // matches both of, and the mapper would send the command once for each. A mapping on
    // either edge covers one on a single edge, so the wider one is kept
    const auto covers = [&destination](const std::pair<Event, Event>& wide, const Event& narrow) {
        return wide.second.str() == destination.str() && wide.second.data() == destination.data() &&
            wide.first.str() == narrow.str() && (wide.first.state() == narrow.state() || wide.first.state() == State::Any);
    };
    const bool covered = std::ranges::any_of(*installed,
        [&covers, &source](const std::pair<Event, Event>& mapped) { return covers(mapped, source); });
    if (covered) {
        return;
    }
    const std::pair<Event, Event> mapping(source, destination);
    std::erase_if(*installed, [&covers, &mapping](const std::pair<Event, Event>& mapped) {
        return covers(mapping, mapped.first) && mapped.second.str() == mapping.second.str() &&
            mapped.second.data() == mapping.second.data();
    });
    installed->push_back(mapping);
}

Event Bindings::rebound(const Event& source, std::string_view command) const {
    // a rebound source keeps the context and the edge the document gave it, and takes only its
    // name from what the player chose. The overrides apply in the order they were made, and
    // each one matches by the document's name, so a later one wins
    const std::string_view named = source.name();
    std::optional<std::string> name;
    for (const Override& made : overrides_) {
        if (made.command == command && (made.source.empty() || made.source == named)) {
            name = made.name;
        }
    }
    if (!name) {
        return source;
    }
    Event replacement(*name, source.context());
    replacement.type(Type::Source);
    replacement.state(source.state());
    return replacement;
}

bool Bindings::readSource(const boost::json::object& mapping, Event* event) {
    const boost::json::object* source = v3d::asset::readObject(mapping, "source");
    if (source == nullptr) {
        logger_->get()->error("Missing mapping source");
        return false;
    }
    if (!source->contains("name") || !source->contains("context")) {
        logger_->get()->error("Mapping source needs both a name and a context");
        return false;
    }
    const std::optional<std::string> name = v3d::asset::readString(*source, "name");
    const std::optional<std::string> context = v3d::asset::readString(*source, "context");
    if (!name || !context) {
        logger_->get()->error("Mapping source needs a name and a context that are strings");
        return false;
    }
    *event = Event(*name, events_->resolveContext(*context));
    event->type(Type::Source);
    // an optional "state" binds one edge only - "pressed"/"down" or "released"/"up". Without
    // it the binding matches both, as most actions need.
    if (source->contains("state")) {
        const std::optional<std::string> state = v3d::asset::readString(*source, "state");
        if (!state) {
            logger_->get()->error("Mapping source state for [{}] is not a string", *name);
            return false;
        }
        event->state(stringToState(*state));
    }
    return true;
}

bool Bindings::readDestination(const boost::json::object& mapping, Event* event) {
    const boost::json::object* destination = v3d::asset::readObject(mapping, "destination");
    if (destination == nullptr) {
        logger_->get()->error("Missing mapping destination");
        return false;
    }
    if (!destination->contains("name") || !destination->contains("context")) {
        logger_->get()->error("Mapping destination needs both a name and a context");
        return false;
    }
    const std::optional<std::string> name = v3d::asset::readString(*destination, "name");
    const std::optional<std::string> context = v3d::asset::readString(*destination, "context");
    if (!name || !context) {
        logger_->get()->error("Mapping destination needs a name and a context that are strings");
        return false;
    }
    *event = Event(*name, events_->resolveContext(*context));
    event->type(Type::Destination);
    // an optional "param" becomes the destination event's data. Several bindings can then drive
    // one command with different values. A menu item's value arrives the same way.
    if (!destination->contains("param")) {
        return true;
    }
    const boost::json::value& param = destination->at("param");
    const std::optional<std::string> text = v3d::asset::readString(*destination, "param");
    if (param.is_int64() || param.is_uint64()) {
        // an integer an int cannot hold would arrive as a different number
        const std::optional<int> number = v3d::asset::readInt(*destination, "param");
        if (!number) {
            logger_->get()->error("The binding param for [{}] is beyond the range of an int", *name);
            return false;
        }
        event->data(*number);
    } else if (param.is_bool()) {
        event->data(param.as_bool());
    } else if (text) {
        event->data(*text);
    } else {
        logger_->get()->error("Unsupported binding param type for [{}]", *name);
        return false;
    }
    return true;
}

};  // namespace v3d::event
