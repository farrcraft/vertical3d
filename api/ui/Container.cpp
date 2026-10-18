/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Container.h"

#include <api/type/geometry/Bound2D.h>
#include <api/ui/DrawOrder.h>

#include "Component.h"

#include <string>
#include <string_view>
#include <vector>

namespace v3d::ui {

namespace {

/**
 * Find a named component in a subtree, the component itself first and then what it holds.
 **/
boost::shared_ptr<Component> search(const boost::shared_ptr<Component>& component, const std::string& name) {
    if (!component) {
        return nullptr;
    }
    if (component->name() == name) {
        return component;
    }
    for (const boost::shared_ptr<Component>& child : component->children()) {
        const boost::shared_ptr<Component> found = search(child, name);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

/**
 * Offer a point to a subtree, deepest and last drawn first, which is the reverse of the
 * order it was drawn in.
 **/
boost::shared_ptr<Component> probe(const boost::shared_ptr<Component>& component, const glm::vec2& point) {
    // a disabled subtree is skipped whole, like a hidden one, because nothing inside a
    // disabled component can be used
    if (!component || !component->visible() || !component->enabled()) {
        return nullptr;
    }

    // only what was drawn can be picked. A tab page that is not chosen keeps the box it last
    // held, and a child drawn over its sibling is offered the point first
    std::vector<boost::shared_ptr<Component>> drawn;
    forEachDrawn(*component, [&drawn](const boost::shared_ptr<Component>& child) { drawn.push_back(child); });
    for (auto it = drawn.rbegin(); it != drawn.rend(); ++it) {
        const boost::shared_ptr<Component> found = probe(*it, point);
        if (found) {
            return found;
        }
    }
    const v3d::type::geometry::Bound2D bound = component->bound();
    if (component->pickable() && bound.contains(point)) {
        return component;
    }
    return nullptr;
}

};  // namespace

Container::Container(const std::string& name, bool visible) :
    name_(name),
    visible_(visible) {
}

void Container::add(const boost::shared_ptr<Component>& component) {
    components_.push_back(component);
}

/**
 **/
boost::shared_ptr<Component> Container::get(const std::string& name) const {
    for (const boost::shared_ptr<Component>& component : components_) {
        const boost::shared_ptr<Component> found = search(component, name);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

std::vector<boost::shared_ptr<Component>> Container::ordered() const {
    return v3d::ui::ordered(components_);
}

boost::shared_ptr<Component> Container::pick(const glm::vec2& point) const {
    if (!visible_) {
        return nullptr;
    }
    // the last thing drawn is the first thing offered the point. A container draws in depth
    // order. Until something is given a depth, that is add order and needs no sorted copy
    std::vector<boost::shared_ptr<Component>> sorted;
    if (!inDrawOrder(components_)) {
        sorted = ordered();
    }
    const std::vector<boost::shared_ptr<Component>>& drawn = sorted.empty() ? components_ : sorted;
    for (auto it = drawn.rbegin(); it != drawn.rend(); ++it) {
        const boost::shared_ptr<Component> found = probe(*it, point);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

bool Container::visible() const {
    return visible_;
}

void Container::visible(bool vis) {
    visible_ = vis;
}

std::string_view Container::name() const {
    return name_;
}


const std::vector<boost::shared_ptr<Component>>& Container::components() const noexcept {
    return components_;
}

};  // namespace v3d::ui
