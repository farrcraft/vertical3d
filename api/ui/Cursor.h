/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <glm/vec2.hpp>

#include <entt/entt.hpp>

namespace v3d::ui {

class Component;
class Container;
class Engine;

/**
 * Turns a cursor into a command, per ADR-0038.
 *
 * A point is offered to the ui in the reverse of the order it was drawn - menu bars first,
 * then toolbars, then the component tree - and the first thing that takes it stops the
 * walk. That order is a fact about how ComponentRenderer draws, which is why it lives here
 * rather than in each app.
 *
 * A press on a pickable component sends that component's bound event and is consumed. A
 * press on anything else is not, so a hud of labels over a scene leaves the scene
 * clickable, which is what ADR-0034's pickable() default of false is for.
 *
 * Everything is tested against the boxes the last draw left on the components, per
 * ADR-0019, so nothing is picked until something has been drawn.
 **/
class Cursor final {
 public:
    /**
     * @param ui the containers to offer a point to, in the order they were loaded
     * @param dispatcher where a picked component's event is sent
     **/
    Cursor(const boost::shared_ptr<Engine>& ui, const boost::shared_ptr<entt::dispatcher>& dispatcher);

    /**
     * The cursor moved.
     *
     * A press being held goes on being followed wherever the cursor is, which is what
     * drags a scrollbar's thumb; otherwise this is what leaves a strip's button hovered.
     *
     * @return whether the ui took it, which is what stops it reaching the scene under it
     **/
    bool motion(const glm::vec2& point);

    /**
     * The primary button went down.
     * @return whether the ui took it
     **/
    bool press(const glm::vec2& point);

    /**
     * The primary button came up, which is what ends a drag. Nothing is dispatched here -
     * a command is sent as the press lands, the way a strip's is.
     * @return whether the ui took it
     **/
    bool release(const glm::vec2& point);

    /**
     * @return the component a press went down on and has not come up from, or null
     **/
    boost::shared_ptr<Component> held() const;

 private:
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

    boost::shared_ptr<Engine> ui_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    // held rather than owned: the component belongs to the container it was loaded into,
    // and a press outliving one that was unloaded should not keep it alive
    boost::weak_ptr<Component> held_;
};

};  // namespace v3d::ui
