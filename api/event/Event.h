/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace v3d::event {


    /**
     **/
    class Event {
     public:
        Event() = default;
        Event(const std::string& name, const std::string& scope, const std::string& context);
        Event(const std::string& name, const std::string& scope);

        bool operator() (const Event& lhs, const Event& rhs) const;
        bool operator <(const Event& rhs) const;

        std::string_view name() const;
        std::string_view scope() const;
        std::string_view context() const;

        std::string str() const;

     private:
        std::string name_;
        std::string scope_;  // aka namespace
        std::string context_;
    };

};  // namespace v3d::event
