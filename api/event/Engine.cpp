/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

#include "Source.h"

#include <boost/make_shared.hpp>

namespace v3d::event {

    Engine::Engine(const boost::shared_ptr<entt::dispatcher>& dispatcher) : dispatcher_(dispatcher) {
        dispatcher->sink<Source>().connect<&Engine::handleSourceEvent>(*this);
    }

    /**
     **/
    void Engine::addMapper(const boost::shared_ptr<Mapper>& mapper) {
        std::string mapperName(mapper->name());
        mappers_[mapperName] = mapper;
    }

    /**
     **/
    void Engine::handleSourceEvent(const Source& source) {
        for (auto it = mappers_.begin(); it != mappers_.end(); it++) {
            boost::optional<Event> destination = it->second->destination(source);
            if (destination) {
                dispatcher_->trigger(destination.get());
            }
        }
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
