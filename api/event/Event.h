/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Context.h"
#include "State.h"
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

    /**
     * Order events by identity and then by state, so that a binding on one edge and a
     * binding on Any are distinct keys in a Mapper rather than colliding.
     **/
    bool operator <(const Event& rhs) const;

    std::string_view name() const;
    boost::shared_ptr<Context> context() const;
    void type(Type t);
    Type type() const;

    /**
     * Set the press/release state. See State.h for what it means on a source event, a
     * binding and a destination event.
     **/
    void state(State s);
    State state() const;

    /**
     * Whether a press is the platform repeating a key that is held down, rather than a new
     * press. A binding carries it from the key to the command. A command that acts while its
     * key is held, such as moving a piece, takes repeats. One that toggles something, such as
     * a menu, ignores them, or holding the key would flick it on and off.
     **/
    void repeat(bool repeated) noexcept;
    bool repeat() const noexcept;

    /**
     * The event's parameter. A binding may configure one, and a menu item carries its
     * value here; either way it reaches the handler as the event's data.
     **/
    void data(const EventData &d);
    boost::optional<EventData> data() const;

    /**
     * The event's identity - "context::name". Does not include the state.
     **/
    std::string str() const;

 private:
    std::string name_;
    boost::shared_ptr<Context> context_;  // aka namespace
    bool hasData_;
    EventData data_;
    Type type_;
    State state_;
    bool repeat_ = false;
};

};  // namespace v3d::event
