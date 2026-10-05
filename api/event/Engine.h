/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Mapper.h"

#include <map>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

#include "Context.h"
#include "Source.h"

namespace v3d::event {
/**
 **/
class Engine {
 public:
     /**
      **/
    explicit Engine(const boost::shared_ptr<entt::dispatcher>& dispatcher);
    // the dispatcher holds a delegate to this object, which a copy or a move would leave
    // pointing at the original
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    /**
     **/
    void addMapper(const boost::shared_ptr<Mapper>& mapper);

    /**
     **/
    void handleSourceEvent(const Unclaimed& unclaimed);

    /**
     * Look up a context from its name.
     * If no existing context exists, a new one will be created.
     * @return context an event context
     **/
    boost::shared_ptr<Context> resolveContext(const std::string_view& name);

 private:
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    std::map<std::string, boost::shared_ptr<Mapper>> mappers_;
    std::vector<boost::shared_ptr<Context>> contexts_;
    // after dispatcher_, so it disconnects before the dispatcher it points into can go
    entt::scoped_connection source_;
};

};  // namespace v3d::event
