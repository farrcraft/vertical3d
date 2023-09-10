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

namespace v3d::event {
    /**
     **/
    class Engine {
     public:
         /**
          **/
        explicit Engine(const boost::shared_ptr<entt::dispatcher>& dispatcher);

        /**
         **/
        void addMapper(const boost::shared_ptr<Mapper>& mapper);

        /**
         **/
        void handleSourceEvent(const Event& source);

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
    };

};  // namespace v3d::event
