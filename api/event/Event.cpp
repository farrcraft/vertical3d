/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Event.h"

namespace v3d::event {

Event::Event(const std::string& name, const std::string& scope, const std::string& context) :
    name_(name), scope_(scope), context_(context), hasData_(false) {
}

Event::Event(const std::string& name, const std::string& scope) :
    name_(name), scope_(scope), hasData_(false) {
}

bool Event::operator() (const Event& lhs, const Event& rhs) const {
    return lhs.str() == rhs.str();
}

bool Event::operator <(const Event& rhs) const {
    return str() == rhs.str();
}

std::string_view Event::name() const {
    return name_;
}

std::string_view Event::scope() const {
    return scope_;
}

std::string_view Event::context() const {
    return context_;
}

std::string Event::str() const {
    using namespace std::literals;
    std::string str = scope_ + "::"s + name_;
    if (context_.length() > 0) {
        str += "::"s + context_;
    }
    return str;
}

void Event::data(const EventData& d) {
    data_ = d;
}

boost::optional<EventData> Event::data() const {
    if (hasData_) {
        return data_;
    }
    return boost::none;
}

};  // namespace v3d::event
