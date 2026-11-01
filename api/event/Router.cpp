/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Router.h"

#include "Chord.h"
#include "Context.h"
#include "Event.h"
#include "Mapper.h"
#include "Source.h"
#include "State.h"

#include <string>
#include <string_view>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::event {

Router::Router(const boost::shared_ptr<entt::dispatcher>& dispatcher) : dispatcher_(dispatcher) {
    source_ = dispatcher->sink<Unclaimed>().connect<&Router::handleSourceEvent>(*this);
}

/**
 **/
void Router::addMapper(const boost::shared_ptr<Mapper>& mapper) {
    const std::string mapperName(mapper->name());
    mappers_[mapperName] = mapper;
}

/**
 **/
void Router::handleSourceEvent(const Unclaimed& unclaimed) {
    const Source& source = *unclaimed.source;
    const std::string key = source.str();
    Modifiers held = source.modifiers();
    if (source.state() == State::Pressed && !source.repeat()) {
        pressed_[key] = held;
    } else {
        const auto down = pressed_.find(key);
        if (down != pressed_.end()) {
            held = down->second;
            if (source.state() == State::Released) {
                pressed_.erase(down);
            }
        }
    }

    for (auto it = mappers_.begin(); it != mappers_.end(); it++) {
        std::vector<Event> destinations = it->second->destinations(source, held);
        for (auto mapped = destinations.begin(); mapped != destinations.end(); ++mapped) {
            // copy the source's edge and repeat flag onto the command, so a handler can tell
            // press from release without a separate binding for each
            mapped->state(source.state());
            mapped->repeat(source.repeat());
            dispatcher_->trigger(*mapped);
        }
    }
}

/**
 **/
boost::shared_ptr<Context> Router::resolveContext(const std::string_view& name) {
    for (auto it = contexts_.begin(); it != contexts_.end(); it++) {
        if ((*it)->name() == name) {
            return *it;
        }
    }
    boost::shared_ptr<Context> context = boost::make_shared<Context>(std::string(name));
    contexts_.push_back(context);
    return context;
}
};  // namespace v3d::event
