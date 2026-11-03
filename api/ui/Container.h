/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Component.h"

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::ui {

/**
 * A named group of top-level components, shown and hidden as a whole. A ui config lists its
 * containers, and they are drawn in that order.
 **/
class Container {
 public:
    explicit Container(const std::string& name, bool visible);
    /**
     * Get the name of the container
     * @return name of the container
     **/
    std::string_view name() const;
    /**
     * Get whether the container is visible or not
     * @return true if the container is visible
     */
    bool visible() const;
    /**
     * Set the visibility state of the container
     * @param vis the new visibility setting
     */
    void visible(bool vis);

    /**
     * Whether the container dims everything drawn before it, as a pause menu or a prompt that
     * waits on an answer does. The dressing's backdrop is laid over the whole canvas before the
     * container's components are drawn. Off unless set.
     **/
    bool backdrop() const noexcept;
    void backdrop(bool dims) noexcept;

    void add(const boost::shared_ptr<Component>& component);
    /**
     * Find a component by name, anywhere in the container - the components it holds and
     * everything they hold in turn.
     * @return the first component of that name in add order, or null when there is none
     **/
    boost::shared_ptr<Component> get(const std::string& name) const;
    /**
     * Get everything the container holds, in the order it was added.
     * @return the components, for a renderer that has to walk all of them
     **/
    const std::vector<boost::shared_ptr<Component>>& components() const noexcept;
    /**
     * Get everything the container holds in the order it is drawn, which is by z index
     * with add order kept between equal depths.
     *
     * Sorted on each call rather than on insertion, because a component's depth can
     * change after it was added. A null that was added is left out.
     *
     * @return the components, lowest depth first, so the last is drawn on top
     **/
    std::vector<boost::shared_ptr<Component>> ordered() const;
    /**
     * The topmost pickable component under a point.
     *
     * Tested against the boxes the components were last drawn in, so nothing is picked
     * until something has been drawn. A child is offered the point before
     * its parent, and a component that is not pickable is passed over without hiding
     * what is under it.
     *
     * @return the component under the point, or null when nothing pickable is there
     **/
    boost::shared_ptr<Component> pick(const glm::vec2& point) const;

 private:
    /**
     * Find a named component in a subtree, the component itself first and then what it holds.
     **/
    static boost::shared_ptr<Component> search(const boost::shared_ptr<Component>& component, const std::string& name);
    /**
     * Offer a point to a subtree, deepest and last drawn first, which is the reverse of the
     * order it was drawn in.
     **/
    static boost::shared_ptr<Component> probe(const boost::shared_ptr<Component>& component, const glm::vec2& point);

    std::string name_;
    std::vector<boost::shared_ptr<Component>> components_;
    bool visible_;
    bool backdrop_;
};

};  // namespace v3d::ui
