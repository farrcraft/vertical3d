/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once


#include <vector>

#include "Tool.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

class ViewPort;

/**
 * Routes the primary button to one tool at a time.
 *
 * A press is offered to each tool in the order they were added, and the first whose takes()
 * answers yes has the gesture. The release that ends it goes to that tool alone, so one press
 * is one tool's gesture from start to end. A tool added later is offered a press only when the
 * ones before it decline. Every tool hears every motion, so a tool can show what a press would
 * do before one happens.
 **/
class ToolRouter final {
 public:
    /**
     * Add a tool behind the ones already added.
     **/
    void add(const boost::shared_ptr<Tool>& tool);

    /**
     * Give every tool the viewport the cursor is in.
     **/
    void view(const boost::shared_ptr<ViewPort>& view);

    /**
     * The primary button went down or came up.
     * @return whether a tool took the press, or the release of a press it took
     **/
    bool button(bool pressed, const glm::vec2& position);

    void motion(const glm::vec2& position);

    /**
     * @return whether any tool has a gesture under way
     **/
    bool dragging() const;

    /**
     * @return the tool that has the gesture, or null between gestures
     **/
    const boost::shared_ptr<Tool>& active() const noexcept;

 private:
    std::vector<boost::shared_ptr<Tool>> tools_;
    boost::shared_ptr<Tool> active_;
};

};  // namespace v3d::editor
