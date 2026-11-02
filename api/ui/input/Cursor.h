/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/paint/Type.h>

#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <glm/vec2.hpp>

#include <entt/entt.hpp>

namespace v3d::ui {

class Component;
class Container;
class Interface;

namespace component {
class TextBox;
class Toolbar;
};  // namespace component

namespace component::menu {
class Bar;
};  // namespace component::menu

};  // namespace v3d::ui

namespace v3d::ui::input {

/**
 * Turns a cursor into a command.
 *
 * A point is offered to the ui in the reverse of the order ComponentRenderer draws: menu
 * bars first, then toolbars, then the component tree. The first thing that takes the point
 * stops the search.
 *
 * A press on a pickable component sends that component's bound event and is consumed. A
 * press on anything else is not. pickable() is false by default, so a hud of labels over a
 * scene leaves the scene clickable.
 *
 * Everything is tested against the boxes the last draw left on the components, so nothing
 * is picked until something has been drawn.
 *
 * A cursor lays text out so that a press inside a text box can place the caret. It takes
 * the same type ComponentRenderer draws the ui's text in. A cursor given none still routes
 * every press, and a text box then keeps its caret where it was.
 **/
class Cursor final {
 public:
    /**
     * @param ui the containers to offer a point to, in the order they were loaded
     * @param dispatcher where a picked component's event is sent
     * @param type what the ui's text is drawn in, so that a point inside a text box becomes a
     *        caret position. Pass the type the renderer drawing this ui was given, so that the
     *        two agree about where a character is
     **/
    Cursor(const boost::shared_ptr<Interface>& ui, const boost::shared_ptr<entt::dispatcher>& dispatcher,
        const paint::Type& type = paint::Type());

    /**
     * The cursor moved.
     *
     * While a press is held, the held component follows the cursor wherever it goes, so a
     * scrollbar's thumb can be dragged. Otherwise this sets which button is hovered, on a
     * toolbar or in the tree.
     *
     * @return whether the ui took it, in which case it does not reach the scene under it
     **/
    bool motion(const glm::vec2& point);

    /**
     * The primary button went down.
     * @return whether the ui took it
     **/
    bool press(const glm::vec2& point);

    /**
     * The primary button came up, ending a held press. The held component follows the cursor
     * to where the button came up, as it would on a motion(), so a slider whose value changes
     * there sends its command. Nothing else is sent, because a command is sent when the press
     * lands, as on a toolbar.
     *
     * A button drawn pressed, in the tree or on a toolbar, goes back to hovered when the cursor
     * is still on it, and to normal when it is not.
     * @return whether the ui took it
     **/
    bool release(const glm::vec2& point);

    /**
     * @return the component a press went down on and has not come up from, or null
     **/
    boost::shared_ptr<Component> held() const;

    /**
     * @return the component in the tree the cursor is over, or null
     **/
    boost::shared_ptr<Component> hovered() const;

 private:
    /**
     * Collect a container's menu bars and toolbars. The cursor offers a point to the menu bars
     * before the toolbars, the reverse of the draw order, because an open menu drops a panel
     * over a toolbar.
     *
     * A strip that is hidden, disabled or not pickable is left out, as Container::pick() leaves
     * out such a component.
     **/
    static void strips(const boost::shared_ptr<Container>& container,
        std::vector<boost::shared_ptr<component::menu::Bar>>* bars,
        std::vector<boost::shared_ptr<component::Toolbar>>* toolbars);
    /**
     * Set a component's hover state on or off.
     *
     * A button is the only component with a hover state. A Toolbar writes the same state onto
     * its buttons, so a button in the tree is highlighted the same way as one on a toolbar.
     *
     * A button that cannot be used is left alone in both directions, so a transient hover never
     * overwrites its disabled look.
     **/
    static void lit(const boost::shared_ptr<Component>& component, bool on);

    /**
     * Highlight one component and clear the highlight on the one before it.
     *
     * A component is hovered when a press would land on it. pickable() decides both what
     * takes a click and what is highlighted, so a hud of labels does not flicker as the
     * cursor crosses it.
     **/
    void hover(const boost::shared_ptr<Component>& component);

    /**
     * Offer a press to one container's menu bars and toolbars, then to what it holds.
     * @return whether anything took it
     **/
    bool press(const boost::shared_ptr<Container>& container, const glm::vec2& point);

    /**
     * Act on a press that landed on a component: choose the row, the tab or the thumb it
     * points at, and send whatever command the component carries.
     **/
    void act(const boost::shared_ptr<Component>& component, const glm::vec2& point);

    /**
     * Send a component's bound event, if it has one bound.
     **/
    void dispatch(const boost::shared_ptr<Component>& component) const;

    /**
     * Carry a held press to where the cursor is now, for a component that is dragged: a
     * scrollbar's thumb, a slider's, or a text box's selection.
     **/
    void follow(const boost::shared_ptr<Component>& holding, const glm::vec2& point) const;

    /**
     * Put a box's caret where a point landed, or take the selection out to there when the
     * press that started it is still down.
     *
     * A cursor with no type leaves the box alone.
     *
     * @param extend whether the anchor stays where it was, as it does during a drag
     **/
    void place(const boost::shared_ptr<component::TextBox>& box, const glm::vec2& point,
        bool extend) const;

    boost::shared_ptr<Interface> ui_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    paint::Type type_;
    // weak, because the component belongs to the container it was loaded into. A held press
    // must not keep alive a component whose container was unloaded
    boost::weak_ptr<Component> held_;
    boost::weak_ptr<Component> hovered_;
};

};  // namespace v3d::ui::input
