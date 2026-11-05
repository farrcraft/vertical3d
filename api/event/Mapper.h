/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Chord.h"
#include "Event.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace v3d::event {
/**
 * A named set of bindings, each from a source event to a destination event.
 **/
class Mapper {
 public:
    explicit Mapper(const std::string& name);

    std::string_view name() const;

    /**
     * Bind a source event to a destination event.
     * One source may be bound to several destinations - an arrow key driving both a
     * paddle and a menu, say - and every one of them is sent when it occurs. A source named
     * as a chord, such as "ctrl+z", is bound to its key with those modifiers.
     **/
    void map(const Event& source, const Event& destination);

    /**
     * Find every destination bound to a source event, with modifiers held.
     *
     * A binding matches when it was bound to the edge the source occurred on, or to
     * State::Any, and every modifier it names is held. Of the bindings that match, only those
     * naming the most modifiers are sent. Ctrl+Z therefore sends what "ctrl+z" is bound to and
     * not what "z" is, while a Ctrl+click sends what "left" is bound to when nothing binds
     * "ctrl+left".
     *
     * @return the matching destinations, empty when the source is bound to nothing
     **/
    std::vector<Event> destinations(const Event& source, const Modifiers& held = Modifiers()) const;

    /**
     * Find every source bound to a destination, whatever edge each was bound on and
     * whatever parameter the destination carries.
     *
     * @param destination the destination as "context::name", the form Event::str() returns
     * @return the sources, empty when nothing is bound to it
     **/
    std::vector<Event> sources(std::string_view destination) const;

    /**
     * @return every destination this mapper sends, as "context::name", each once and in name
     *         order
     **/
    std::vector<std::string> destinations() const;

 private:
    /**
     * One binding: what it sends, the modifiers it needs held, and the source as it was bound,
     * chord and all, for sources() to answer with.
     **/
    struct Binding final {
        Event destination;
        Modifiers modifiers;
        Event source;
    };

    // keyed by the source's key alone, so every chord on one key sits beside its plain binding
    std::multimap<Event, Binding> mappings_;
    std::string name_;
};
};  // namespace v3d::event
