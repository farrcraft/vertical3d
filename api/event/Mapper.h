/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Event.h"

#include <map>
#include <string>

#include <boost/optional.hpp>

namespace v3d::event {
    /**
     **/
    class Mapper {
     public:
        explicit Mapper(const std::string& name);

        std::string_view name() const;

        void map(const Event& source, const Event& destination);
        boost::optional<Event> destination(const Event& source);

     private:
        std::string name_;
        std::map<Event, Event> mappings_;
    };
};  // namespace v3d::event
