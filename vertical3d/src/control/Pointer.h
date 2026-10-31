/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/ui/input/Cursor.h>
#include <vertical3d/src/tool/ToolRouter.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ViewPort.h>

#include <functional>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * Routes the mouse: to the ui drawn over the views, to a divider between areas, and to the
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
    Pointer(const boost::shared_ptr<v3d::ui::input::Cursor>& ui, const boost::shared_ptr<AreaViews>& views,
        const boost::shared_ptr<ToolRouter>& tools, const Moved& moved);

    /**
     * The cursor moved, in window pixels.
     **/
    void motion(const glm::vec2& cursor);

    /**
     * The primary button went down or up, where the cursor is.
     **/
    void button(bool pressed);

    /**
     * Divide a region of the window between the areas, and tell each view its region.
     * @param region x, y, width and height, in window pixels
     **/
    void resize(const glm::vec4& region);

    /**
     * Tell each view the region the layout gives it, after a change to the layout. A view that
     * is no longer shown stops being the active one, and the first view shown takes its place.
     **/
    void place();

    const glm::vec2& cursor() const noexcept;

    /**
     * @return the view the cursor was last in, or the first view shown before the cursor has
     *         moved
     **/
    const boost::shared_ptr<ViewPort>& activeView() const noexcept;

 private:
    /**
     * Make a view the active one, and tell the tools and the caller.
     **/
    void activate(const boost::shared_ptr<ViewPort>& view);

    boost::shared_ptr<v3d::ui::input::Cursor> ui_;
    boost::shared_ptr<AreaViews> views_;
    boost::shared_ptr<ToolRouter> tools_;
    Moved moved_;
    boost::shared_ptr<ViewPort> activeView_;
    glm::vec2 cursor_;
    // whether the ui took the press, so that the release that ends it does not reach the tools
    // that never saw the press
    bool uiGrab_;
};

};  // namespace v3d::editor
