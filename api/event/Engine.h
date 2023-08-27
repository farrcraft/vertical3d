/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Mapper.h"

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

#include "Source.h"

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
        void handleSourceEvent(const Source& source);

     private:
        boost::shared_ptr<entt::dispatcher> dispatcher_;
        std::map<std::string, boost::shared_ptr<Mapper>> mappers_;
    };

};  // namespace v3d::event
