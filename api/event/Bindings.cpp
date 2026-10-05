/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bindings.h"

#include <string>
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
    rebindings_[command] = name;
    return build();
}

std::vector<Event> Bindings::sources(std::string_view command) const {
    if (!mapper_) {
        return std::vector<Event>();
    }
    return mapper_->sources(command);
}

bool Bindings::build() {
    // every lookup below is guarded by a contains() rather than reaching straight for at():
    // boost::json::at throws, and a document this does not understand has to come back as a
    // false return, not as an exception out of startup
    const boost::json::object& doc = *document_;
    if (!doc.contains("mappings") || !doc.at("mappings").is_array()) {
        logger_->get()->error("Missing mappings in config");
        return false;
    }

    boost::shared_ptr<Mapper> mapper = boost::make_shared<Mapper>("global");
    for (const boost::json::value& item : doc.at("mappings").as_array()) {
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
        // a rebound command keeps the context and the edge the document gave it, and takes
        // only its name from what the player chose
        const std::map<std::string, std::string>::const_iterator rebound = rebindings_.find(destination.str());
        if (rebound != rebindings_.end()) {
            Event replacement(rebound->second, source.context());
            replacement.type(Type::Source);
            replacement.state(source.state());
            source = replacement;
        }
        if (known_ && !known_(source)) {
            logger_->get()->warn("{} is bound to {}, which no device sends, so it never fires", destination.str(), source.str());
        }
        mapper->map(source, destination);
    }
    // addMapper stores by name, so this replaces the mapper rather than adding a second
    events_->addMapper(mapper);
    mapper_ = mapper;
    return true;
}

bool Bindings::readSource(const boost::json::object& mapping, Event* event) {
    if (!mapping.contains("source") || !mapping.at("source").is_object()) {
        logger_->get()->error("Missing mapping source");
        return false;
    }
    const boost::json::object& source = mapping.at("source").as_object();
    if (!source.contains("name") || !source.contains("context")) {
        logger_->get()->error("Mapping source needs both a name and a context");
        return false;
    }
    const std::string name = boost::json::value_to<std::string>(source.at("name"));
    const std::string context = boost::json::value_to<std::string>(source.at("context"));
    *event = Event(name, events_->resolveContext(context));
    event->type(Type::Source);
    // an optional "state" binds one edge only - "pressed"/"down" or "released"/"up". Without
    // it the binding matches both, which is what most actions want.
    if (source.contains("state")) {
        event->state(stringToState(boost::json::value_to<std::string>(source.at("state"))));
    }
    return true;
}

bool Bindings::readDestination(const boost::json::object& mapping, Event* event) {
    if (!mapping.contains("destination") || !mapping.at("destination").is_object()) {
        logger_->get()->error("Missing mapping destination");
        return false;
    }
    const boost::json::object& destination = mapping.at("destination").as_object();
    if (!destination.contains("name") || !destination.contains("context")) {
        logger_->get()->error("Mapping destination needs both a name and a context");
        return false;
    }
    const std::string name = boost::json::value_to<std::string>(destination.at("name"));
    const std::string context = boost::json::value_to<std::string>(destination.at("context"));
    *event = Event(name, events_->resolveContext(context));
    event->type(Type::Destination);
    // an optional "param" lets one action serve several bindings, telling them apart by the
    // value it arrives with. It reaches the handler as the event's data, the same way a menu
    // item's value does.
    if (!destination.contains("param")) {
        return true;
    }
    const boost::json::value& param = destination.at("param");
    if (param.is_int64()) {
        event->data(static_cast<int>(param.as_int64()));
    } else if (param.is_bool()) {
        event->data(param.as_bool());
    } else if (param.is_string()) {
        event->data(boost::json::value_to<std::string>(param));
    } else {
        logger_->get()->error("Unsupported binding param type for [{}]", name);
        return false;
    }
    return true;
}

};  // namespace v3d::event
