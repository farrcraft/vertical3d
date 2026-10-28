/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

class ViewPort;

/**
 * Something the user does with the mouse in a viewport.
 *
 * A tool is activated by name, as a command is invoked, and it also receives motion and
 * button events. A ToolRouter offers it presses in turn with the other tools.
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
     * The viewport the tool works in, set as the cursor moves between viewports.
     **/
    virtual void view(const boost::shared_ptr<ViewPort>& view) = 0;

    /**
     * @return whether a press at a point, in window pixels, would start this tool's gesture
     **/
    virtual bool takes(const glm::vec2& position) const = 0;

    /**
     * @return whether a gesture of this tool is under way
     **/
    virtual bool dragging() const noexcept = 0;

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
