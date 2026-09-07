/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Container.h"

#include <string>
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
    if (!component || !component->visible()) {
        return nullptr;
    }
    const std::vector<boost::shared_ptr<Component>>& children = component->children();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        const boost::shared_ptr<Component> found = probe(*it, point);
        if (found) {
            return found;
        }
    }
    v3d::type::Bound2D bound = component->bound();
    if (component->pickable() && bound.intersect(point)) {
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
    // the last thing drawn is the first thing offered the point, and a container draws in
    // depth order
    const std::vector<boost::shared_ptr<Component>> sorted = ordered();
    for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
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
