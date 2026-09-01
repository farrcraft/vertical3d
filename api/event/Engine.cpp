/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::event {

    Engine::Engine(const boost::shared_ptr<entt::dispatcher>& dispatcher) : dispatcher_(dispatcher) {
        dispatcher->sink<Event>().connect<&Engine::handleSourceEvent>(*this);
    }

    /**
     **/
    void Engine::addMapper(const boost::shared_ptr<Mapper>& mapper) {
        std::string mapperName(mapper->name());
        mappers_[mapperName] = mapper;
    }

    /**
     **/
    void Engine::handleSourceEvent(const Event& source) {
        if (source.type() != Type::Source) {
            return;
        }
        for (auto it = mappers_.begin(); it != mappers_.end(); it++) {
            std::vector<Event> destinations = it->second->destinations(source);
            for (auto mapped = destinations.begin(); mapped != destinations.end(); ++mapped) {
                // carry the edge across so a handler can tell press from release without
                // needing a separate binding for each
                mapped->state(source.state());
                dispatcher_->trigger(*mapped);
            }
        }
    }

    /**
     **/
    void Engine::dispatch(const std::string_view& context, const std::string& name) {
        Event event(name, resolveContext(context));
        event.type(Type::Destination);
        dispatcher_->trigger(event);
    }

    /**
     **/
    void Engine::dispatch(const std::string_view& context, const std::string& name, const EventData& data) {
        Event event(name, resolveContext(context));
        event.type(Type::Destination);
        event.data(data);
        dispatcher_->trigger(event);
    }

    /**
     **/
    boost::shared_ptr<Context> Engine::resolveContext(const std::string_view& name) {
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
