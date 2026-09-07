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
     * change after it was added.
     *
     * @return the components, deepest first
     **/
    std::vector<boost::shared_ptr<Component>> ordered() const;
    /**
     * The topmost pickable component under a point.
     *
     * Tested against the boxes the components were last drawn in, so nothing is picked
     * until something has been drawn, per ADR-0019. A child is offered the point before
     * its parent, and a component that is not pickable is passed over without hiding
     * what is under it.
     *
     * @return the component under the point, or null when nothing pickable is there
     **/
    boost::shared_ptr<Component> pick(const glm::vec2& point) const;

 private:
    std::string name_;
    std::vector<boost::shared_ptr<Component>> components_;
    bool visible_;
};

};  // namespace v3d::ui
