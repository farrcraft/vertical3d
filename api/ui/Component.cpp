/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Component.h"

#include <algorithm>
#include <string>
#include <vector>

namespace v3d::ui {

unsigned int Component::lastID = 0;


Component::Component(component::Type type) :
    parent_(nullptr),
    id_(lastID++),
    visible_(true),
    pickable_(false),
    zIndex_(0),
    type_(type),
    size_(0.0f, 0.0f),
    position_(0.0f, 0.0f) {
}

Component::~Component() {
}

bool Component::visible() const {
    return visible_;
}

void Component::visible(bool vis) {
    visible_ = vis;
}

unsigned int Component::id() const {
    return id_;
}

void Component::position(const glm::vec2& pos) {
    position_ = pos;
}

void Component::size(const glm::vec2& s) {
    size_ = s;
}

glm::vec2 Component::position() const {
    return position_;
}

glm::vec2 Component::size() const {
    return size_;
}

unsigned int Component::depth() const {
    return zIndex_;
}

void Component::depth(unsigned int index) {
    zIndex_ = index;
}

Layout& Component::layout() noexcept {
    return layout_;
}

const Layout& Component::layout() const noexcept {
    return layout_;
}

void Component::add(const boost::shared_ptr<Component>& child) {
    if (!child || child->parent_ != nullptr) {
        return;
    }
    child->parent_ = this;
    children_.push_back(child);
}

const std::vector<boost::shared_ptr<Component>>& Component::children() const noexcept {
    return children_;
}

Component* Component::parent() const noexcept {
    return parent_;
}

bool Component::pickable() const {
    return pickable_;
}

void Component::pickable(bool pick) {
    pickable_ = pick;
}

v3d::type::Bound2D Component::bound() const {
    v3d::type::Bound2D bound(position_, size_);
    return bound;
}

std::string_view Component::style() const {
    return style_;
}

void Component::style(const std::string& str) {
    style_ = str;
}

std::string_view Component::name() const {
    return name_;
}

void Component::name(const std::string& str) {
    name_ = str;
}
component::Type Component::type() const {
    return type_;
}

std::vector<boost::shared_ptr<Component>> ordered(const std::vector<boost::shared_ptr<Component>>& components) {
    std::vector<boost::shared_ptr<Component>> sorted(components);
    std::stable_sort(sorted.begin(), sorted.end(),
        [](const boost::shared_ptr<Component>& first, const boost::shared_ptr<Component>& second) {
            return first->depth() < second->depth();
        });
    return sorted;
}

};  // end namespace v3d::ui
