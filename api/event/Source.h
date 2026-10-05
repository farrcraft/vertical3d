/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Context.h"
#include "Event.h"
#include "State.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

namespace v3d::event {

/**
 * What a device sent - a key or a button going down or up - as against the command a binding
 * makes of it, which is an Event.
 *
 * A type of its own so that the two are two sinks: a listener on sink<Event> hears commands
 * and nothing else, and one that wants the keys themselves, as a key capture does, asks for
 * sink<Source> - ADR-0081. It is an Event underneath because a binding is written in the same
 * terms for both ends, which is what a Mapper keys on.
 **/
class Source final : public Event {
 public:
    Source(const std::string& name, const boost::shared_ptr<Context>& context, State state);

    /**
     * Take the key for the listener that heard it, so its bindings make nothing of it - what
     * a key capture does, since the key is the answer and not a command. A listener hears a
     * source as a const reference, which is why this is const.
     **/
    void consume() const noexcept;

    bool consumed() const noexcept;

 private:
    // shared, because a dispatcher hands each listener a copy of what it was given, and
    // publish() asks the one it sent
    boost::shared_ptr<bool> consumed_;
};

/**
 * A source no listener consumed, for the event engine to turn into the commands it is bound
 * to. Only the event engine listens for this.
 **/
struct Unclaimed final {
    const Source* source;  // never null; a pointer because a dispatcher stores its events by value
};

/**
 * Send a source to every listener, and then, unless one consumed it, the commands its
 * bindings make of it.
 *
 * So every listener has heard the key before any hears what it was bound to, whatever order
 * they connected in - a dispatcher calls a sink's listeners in reverse order of connection,
 * which is why the event engine is not one of them. This is the one place a source is sent
 * from - ADR-0081.
 **/
void publish(entt::dispatcher& dispatcher, const Source& source);

};  // namespace v3d::event
