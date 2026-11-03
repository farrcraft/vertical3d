/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace v3d::event {
/**
 * The namespace an event's name belongs to, such as "keyboard" for a key or "pong" for a
 * command. An event's identity is "context::name".
 **/
class Context {
 public:
    explicit Context(const std::string& name);

    std::string_view name() const;

 private:
    std::string name_;
};

};  // namespace v3d::event
