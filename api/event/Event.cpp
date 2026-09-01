/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Event.h"

#include <string>

namespace v3d::event {

Event::Event(const std::string& name, const boost::shared_ptr<Context>& context) :
    name_(name), context_(context), hasData_(false), type_(Type::Unknown), state_(State::Any) {
}

Event::Event(const std::string& name) :
    name_(name), hasData_(false), type_(Type::Unknown), state_(State::Any) {
}

bool Event::operator() (const Event& lhs, const Event& rhs) const {
    return lhs.str() == rhs.str();
}

bool Event::operator <(const Event& rhs) const {
    int order = str().compare(rhs.str());
    if (order != 0) {
        return order < 0;
    }
    return state_ < rhs.state_;
}

std::string_view Event::name() const {
    return name_;
}

boost::shared_ptr<Context> Event::context() const {
    return context_;
}

/**
 **/
void Event::type(Type t) {
    type_ = t;
}

/**
 **/
Type Event::type() const {
    return type_;
}

/**
 **/
void Event::state(State s) {
    state_ = s;
}

/**
 **/
State Event::state() const {
    return state_;
}

std::string Event::str() const {
    using namespace std::literals;
    if (!context_) {
        return name_;
    }
    std::string str = std::string(context_->name()) + "::"s + name_;
    return str;
}

void Event::data(const EventData& d) {
    hasData_ = true;
    data_ = d;
}

boost::optional<EventData> Event::data() const {
    if (hasData_) {
        return data_;
    }
    return boost::none;
}

};  // namespace v3d::event
