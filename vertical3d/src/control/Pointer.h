/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/ui/input/Cursor.h>
#include <vertical3d/src/tool/ToolRouter.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <functional>
#include <optional>

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
 * a toolbar is not interrupted. The area under the cursor becomes the active one between drags,
 * and its view the active view, so a drag that crosses a gap keeps its area.
 *
 * A divider is taken by a press near its gap. A drag moves it, snapped to tenths while Ctrl is
 * held, and cancel() puts it back. The second press of a double click on a divider evens it
 * instead of taking it.
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
     * @param snap whether a held divider snaps to tenths of its split
     **/
    void motion(const glm::vec2& cursor, bool snap = false);

    /**
     * The primary button went down or up, where the cursor is.
     * @param clicks how many presses in quick succession this one is
     **/
    void button(bool pressed, unsigned int clicks = 1);

    /**
     * Put a held divider back where it was taken, and let go of it.
     * @return whether a divider was held
     **/
    bool cancel();

    /**
     * @return the divider to light: the one held, or else the one a press would take, unless
     *         the ui has the cursor or a tool is dragging
     **/
    std::optional<AreaLayout::Divider> lit() const;

    /**
     * @return the index of the area the cursor was last in, or of the first area before the
     *         cursor has been in one
     **/
    std::size_t activeArea() const noexcept;

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
    // the id of the area the cursor was last in
    std::optional<std::size_t> activeArea_;
    glm::vec2 cursor_;
    // whether the ui took the press, so that the release that ends it does not reach the tools
    // that never saw the press
    bool uiGrab_;
    // whether a divider took the press, for the same reason
    bool dividerGrab_ = false;
    // whether the ui took the last motion
    bool overUi_ = false;
};

};  // namespace v3d::editor
