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
     * Order events by identity, then by state. A Mapper keeps its bindings in this order, so
     * the bindings on one source sit together, sorted by edge. The parameter is not compared.
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
     * Whether a press is the platform repeating a key that is held down, not a new press. The
     * event router copies it from the key to each command the key is bound to. A command that
     * acts while its key is held, such as moving a piece, takes repeats. A command that toggles
     * something, such as a menu, ignores them, or holding the key would flick it on and off.
     **/
    void repeat(bool repeated) noexcept;
    bool repeat() const noexcept;

    /**
     * The event's parameter. A binding's "param" and a menu item's value both reach the
     * handler here.
     **/
    void data(const EventData &payload);
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
