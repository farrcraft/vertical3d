/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Container.h"

namespace v3d::ui {

Container::Container(const std::string& name) :
    name_(name),
    visible_(false) {
}

void Container::add(const boost::shared_ptr<Component>& component) {
    components_.push_back(component);
}

bool Container::visible() const {
    return visible_;
}

void Container::visible(bool vis) {
    visible_ = vis;
}

};  // namespace v3d::ui
