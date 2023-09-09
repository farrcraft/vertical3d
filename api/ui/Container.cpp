/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Container.h"

namespace v3d::ui {

Container::Container(const std::string& name, bool visible) :
    name_(name),
    visible_(visible) {
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

std::string_view Container::name() const {
    return name_;
}


};  // namespace v3d::ui
