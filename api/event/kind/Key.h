/*
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/event/Event.h>

#include <string>

/**
 * The kinds of event: keys, the mouse, text input, sounds, and changes to the window.
 **/
namespace v3d::event::kind {

/**
 * A key changed edge. The event's name is the key's name, such as "escape".
 **/
class Key : public Event {
 public:
    /**
     **/
    Key(const std::string& name, const boost::shared_ptr<Context>& context, bool pressed) noexcept;

    /**
     **/
    bool pressed() const noexcept;

 private:
    bool pressed_;
};
};  // namespace v3d::event::kind
