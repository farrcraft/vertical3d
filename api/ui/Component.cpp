/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Component.h"

namespace v3d::ui {

    unsigned int Component::lastID = 0;


    Component::Component() : id_(lastID++), visible_(true), zIndex_(0) {
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

    v3d::type::Bound2D Component::bound() const {
        v3d::type::Bound2D bound(position_, size_);
        return bound;
    }

    std::string Component::style() const {
        return style_;
    }

    void Component::style(const std::string& str) {
        style_ = str;
    }

    std::string Component::name() const {
        return name_;
    }

    void Component::name(const std::string& str) {
        name_ = str;
    }

};  // end namespace v3d::ui
