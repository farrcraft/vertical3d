/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Component.h>
#include <api/ui/component/Box.h>
#include <api/ui/component/TabBar.h>

#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::ui {

/**
 * Visit what a component holds that is live, in the order it is drawn.
 *
 * A tab bar holds every page and draws only the chosen one; a flow box draws its children in
 * the order it holds them, because that order is what it lays out; anything else draws by
 * depth. This is the one statement of that rule, and the draw walk, the pick and the tab order
 * all go through it - so a control on a page nobody can see is neither picked nor focused, and
 * a child is picked where it was drawn (ADR-0019).
 *
 * @param visit called with each live child, as a boost::shared_ptr<Component>
 **/
template <typename Visit>
void forEachDrawn(const Component& component, Visit&& visit) {
    const component::Traits kind = component::traits(component.type());
    if (kind.pages) {
        const boost::shared_ptr<Component> page = static_cast<const component::TabBar&>(component).page();
        if (page) {
            visit(page);
        }
        return;
    }
    const std::vector<boost::shared_ptr<Component>>& children = component.children();
    if (kind.flow || inDrawOrder(children)) {
        for (const boost::shared_ptr<Component>& child : children) {
            visit(child);
        }
        return;
    }
    for (const boost::shared_ptr<Component>& child : ordered(children)) {
        visit(child);
    }
}

};  // namespace v3d::ui
