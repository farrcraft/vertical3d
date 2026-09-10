/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>

#include <string>

namespace v3d::event::kind {

/**
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
