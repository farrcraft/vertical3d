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

/**
 **/
boost::shared_ptr<Component> Container::get(const std::string& name) const {
    std::vector<boost::shared_ptr<Component>>::const_iterator it = components_.begin();
    for (; it != components_.end(); it++) {
        if ((*it)->name() == name) {
            return (*it);
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


};  // namespace v3d::ui
