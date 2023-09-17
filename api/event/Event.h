/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Context.h"
#include "Type.h"

#include <string>
#include <variant>

#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

namespace v3d::event {

    using EventData = std::variant<int, bool, std::string>;

    /**
     **/
    class Event {
     public:
        Event() = default;
        Event(const std::string& name, const boost::shared_ptr<Context>& context);
        explicit Event(const std::string& name);

        bool operator() (const Event& lhs, const Event& rhs) const;
        bool operator <(const Event& rhs) const;

        std::string_view name() const;
        boost::shared_ptr<Context> context() const;
        void type(Type t);
        constexpr Type type() const;

        void data(const EventData &d);
        boost::optional<EventData> data() const;

        std::string str() const;

     private:
        std::string name_;
        boost::shared_ptr<Context> context_;  // aka namespace
        bool hasData_;
        EventData data_;
        Type type_;
    };

};  // namespace v3d::event
