/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/ui/input/Cursor.h>
#include <vertical3d/src/tool/ToolRouter.h>
#include <vertical3d/src/view/ViewLayout.h>
#include <vertical3d/src/view/ViewPort.h>

#include <functional>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * Routes the mouse: to the ui drawn over the views, to a divider between views, and to the
 * tools of the view under the cursor.
 *
 * A press goes to the first of those that takes it, and its release goes to the same place.
 * The ui is offered the cursor only between drags, so a drag that passes over the menu bar or
 * a toolbar is not interrupted. The view under the cursor becomes the active one between drags,
 * so a drag that crosses a border between views keeps its view.
 **/
class Pointer final {
 public:
    /**
     * Called when the cursor moves into another view.
     **/
    typedef std::function<void(const boost::shared_ptr<ViewPort>& view)> Moved;

    /**
     * @param ui what is offered the cursor first, which may be null for no ui
     * @param moved what to tell when the active view changes
     **/
    Pointer(const boost::shared_ptr<v3d::ui::input::Cursor>& ui, const boost::shared_ptr<ViewLayout>& layout,
        const std::vector<boost::shared_ptr<ViewPort>>& views, const boost::shared_ptr<ToolRouter>& tools,
        const Moved& moved);

    /**
     * The cursor moved, in window pixels.
     **/
    void motion(const glm::vec2& cursor);

    /**
     * The primary button went down or up, where the cursor is.
     **/
    void button(bool pressed);

    /**
     * Divide an area of the window between the views, and tell each one its region.
     * @param area x, y, width and height, in window pixels
     **/
    void resize(const glm::vec4& area);

    const glm::vec2& cursor() const noexcept;

    /**
     * @return the view the cursor was last in, or the first view before the cursor has moved
     **/
    const boost::shared_ptr<ViewPort>& activeView() const noexcept;

 private:
    /**
     * Tell each view the region the layout last gave it, as after a divider is dragged.
     **/
    void place();

    boost::shared_ptr<v3d::ui::input::Cursor> ui_;
    boost::shared_ptr<ViewLayout> layout_;
    std::vector<boost::shared_ptr<ViewPort>> views_;
    boost::shared_ptr<ToolRouter> tools_;
    Moved moved_;
    boost::shared_ptr<ViewPort> activeView_;
    glm::vec2 cursor_;
    // whether the ui took the press, so that the release that ends it does not reach the tools
    // that never saw the press
    bool uiGrab_;
};

};  // namespace v3d::editor
