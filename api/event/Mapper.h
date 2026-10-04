/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Event.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace v3d::event {
/**
 **/
class Mapper {
 public:
    explicit Mapper(const std::string& name);

    std::string_view name() const;

    /**
     * Bind a source event to a destination event.
     * One source may be bound to several destinations - an arrow key driving both a
     * paddle and a menu, say - and every one of them is sent when it occurs.
     **/
    void map(const Event& source, const Event& destination);

    /**
     * Find every destination bound to a source event.
     * A binding matches when it was bound to the edge the source occurred on, or to
     * State::Any.
     *
     * @return the matching destinations, empty when the source is bound to nothing
     **/
    std::vector<Event> destinations(const Event& source) const;

    /**
     * Find every source bound to a destination, whatever edge each was bound on and
     * whatever parameter the destination carries.
     *
     * @param destination the destination as "context::name", which is how Event::str()
     *        and a rebind name one
     * @return the sources, empty when nothing is bound to it
     **/
    std::vector<Event> sources(std::string_view destination) const;

 private:
    std::multimap<Event, Event> mappings_;
    std::string name_;
};
};  // namespace v3d::event
