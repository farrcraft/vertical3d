/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/paint/Text.h>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <glm/vec2.hpp>

#include <entt/entt.hpp>

namespace v3d::ui {

class Component;
class Container;
class Engine;

namespace component {
class TextBox;
};  // namespace component

};  // namespace v3d::ui

namespace v3d::ui::input {

/**
 * Turns a cursor into a command.
 *
 * A point is offered to the ui in the reverse of the order it was drawn - menu bars first,
 * then toolbars, then the component tree - and the first thing that takes it stops the
 * search. That order follows how ComponentRenderer draws, so it lives here rather than in
 * each app.
 *
 * A press on a pickable component sends that component's bound event and is consumed. A
 * press on anything else is not. pickable() is false by default, so a hud of labels over a
 * scene leaves the scene clickable.
 *
 * Everything is tested against the boxes the last draw left on the components, so nothing
 * is picked until something has been drawn.
 *
 * A cursor measures text so that a press inside a text box can place the caret. It takes
 * the same Measure both renderers take. A cursor given none still routes every press, and a
 * text box then keeps the caret it had rather than taking one from where it was clicked.
 **/
class Cursor final {
 public:
    /**
     * @param ui the containers to offer a point to, in the order they were loaded
     * @param dispatcher where a picked component's event is sent
     * @param measure how wide a run of text is when the app draws it, so that a point
     *        inside a text box becomes a caret position. Pass the callback the renderer
     *        drawing this ui was given, so that the two agree about where a character is
     **/
    Cursor(const boost::shared_ptr<Engine>& ui, const boost::shared_ptr<entt::dispatcher>& dispatcher,
        const paint::Measure& measure = paint::Measure());

    /**
     * The cursor moved.
     *
     * A press being held goes on being followed wherever the cursor is, so a scrollbar's
     * thumb follows it. Otherwise this sets which button is hovered, whether it sits on a
     * strip or in the tree.
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
     * The primary button came up, ending a drag. Nothing is dispatched here -
     * a command is sent as the press lands, the way a strip's is.
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
     * Light one component up and put back whatever was lit before it.
     *
     * A component is hovered when it is the one a press would land on, so the same
     * pickable() that decides what takes a click decides what lights up, so a hud of labels
     * does not flicker as the cursor crosses it.
     **/
    void hover(const boost::shared_ptr<Component>& component);

    /**
     * Offer a press to one container's strips, then to what it holds.
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
     * A cursor with no Measure names no text and leaves the box alone.
     *
     * @param extend whether the anchor stays where it was, as it does during a drag
     **/
    void place(const boost::shared_ptr<component::TextBox>& box, const glm::vec2& point,
        bool extend) const;

    boost::shared_ptr<Engine> ui_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    paint::Measure measure_;
    // held rather than owned: the component belongs to the container it was loaded into,
    // and a press outliving one that was unloaded should not keep it alive
    boost::weak_ptr<Component> held_;
    boost::weak_ptr<Component> hovered_;
};

};  // namespace v3d::ui::input
