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
class Router {
 public:
     /**
      **/
    explicit Router(const boost::shared_ptr<entt::dispatcher>& dispatcher);
    // the dispatcher holds a delegate to this object, which a copy or a move would leave
    // pointing at the original
    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    /**
     **/
    void addMapper(const boost::shared_ptr<Mapper>& mapper);

    /**
     **/
    void handleSourceEvent(const Unclaimed& unclaimed);

    /**
     * Look up a context from its name.
     * Creates the context when none has that name.
     * @return context an event context
     **/
    boost::shared_ptr<Context> resolveContext(const std::string_view& name);

 private:
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    std::map<std::string, boost::shared_ptr<Mapper>> mappers_;
    std::vector<boost::shared_ptr<Context>> contexts_;
    // declared after dispatcher_, so it is destroyed first and disconnects while the
    // dispatcher still exists
    entt::scoped_connection source_;
};

};  // namespace v3d::event
