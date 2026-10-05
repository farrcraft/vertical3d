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
 * A type of its own so that the two go to separate sinks: a listener on sink<Event> receives
 * commands only, and one that needs the keys themselves, such as a key capture, listens on
 * sink<Source>. It derives from Event because a binding names both ends in the same terms, and
 * a Mapper keys on them.
 **/
class Source final : public Event {
 public:
    Source(const std::string& name, const boost::shared_ptr<Context>& context, State state);

    /**
     * Consume the key, so its bindings send no command. A key capture does this, because it
     * needs the key itself rather than a command. A listener receives a source as a const
     * reference, so this is const.
     **/
    void consume() const noexcept;

    bool consumed() const noexcept;

 private:
    // shared, because a dispatcher gives each listener a copy, and publish() reads the flag
    // on the one it sent
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
 * Send a source to every listener on sink<Source>, and then, unless one consumed it, send the
 * commands its bindings make of it.
 *
 * Every listener receives the key before any receives its command, whatever order they
 * connected in. A dispatcher calls a sink's listeners in reverse order of connection, so the
 * event engine listens for Unclaimed rather than for the key. A source is always sent through
 * this function and never triggered directly.
 **/
void publish(entt::dispatcher& dispatcher, const Source& source);

};  // namespace v3d::event
