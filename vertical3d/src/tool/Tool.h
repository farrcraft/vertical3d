/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>

#include <glm/vec2.hpp>

namespace v3d::editor {

/**
 * Something the user does with the mouse in a viewport.
 *
 * The interactive half of the command model: a command that also receives motion and
 * button events for as long as it is the active one.
 **/
class Tool {
 public:
    virtual ~Tool() { }

    /**
     * Become the active tool. The name is which of the tool's modes was asked for, so
     * one tool can serve several bindings.
     **/
    virtual void activate(const std::string& name) = 0;

    /**
     * Stop being the active tool.
     **/
    virtual void deactivate(const std::string& name) = 0;

    /**
     * The cursor moved, in window pixels.
     **/
    virtual void motion(const glm::vec2& position) = 0;

    /**
     * A mouse button went down or came up.
     **/
    virtual void button(unsigned int button, bool pressed, const glm::vec2& position) = 0;
};


};  // namespace v3d::editor
