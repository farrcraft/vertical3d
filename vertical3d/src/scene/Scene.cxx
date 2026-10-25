/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scene.h"

#include "Camera.h"
#include "Item.h"
#include "Light.h"
#include "Mesh.h"
#include "SceneVisitor.h"

#include <algorithm>
#include <cstddef>
#include <iterator>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
unsigned int Scene::add(const boost::shared_ptr<Item>& item) {
    if (!item) {
        return 0;
    }
    items_.push_back(item);
    return item->id();
}

/**
 **/
bool Scene::remove(unsigned int id) {
    auto it = std::find_if(items_.begin(), items_.end(),
        [id](const boost::shared_ptr<Item>& item) { return item->id() == id; });
    if (it == items_.end()) {
        return false;
    }
    items_.erase(it);
    return true;
}

/**
 **/
boost::shared_ptr<Item> Scene::item(unsigned int id) const {
    auto it = std::find_if(items_.begin(), items_.end(),
        [id](const boost::shared_ptr<Item>& item) { return item->id() == id; });
    if (it == items_.end()) {
        return boost::shared_ptr<Item>();
    }
    return *it;
}

/**
 **/
boost::shared_ptr<Mesh> Scene::mesh(unsigned int id) const {
    return as<Mesh>(item(id));
}

/**
 **/
Items Scene::selection() const {
    Items selected;
    std::copy_if(items_.begin(), items_.end(), std::back_inserter(selected),
        [](const boost::shared_ptr<Item>& item) { return item->selected(); });
    return selected;
}

/**
 **/
std::size_t Scene::count() const noexcept {
    return items_.size();
}

/**
 **/
void Scene::clear() noexcept {
    items_.clear();
}

/**
 **/
void Scene::accept(SceneVisitor* visitor) const {
    if (visitor == nullptr) {
        return;
    }
    for (const boost::shared_ptr<Item>& item : items_) {
        switch (item->kind()) {
            case Item::Kind::Mesh:
                visitor->visit(boost::static_pointer_cast<Mesh>(item));
                break;
            case Item::Kind::Camera:
                visitor->visit(boost::static_pointer_cast<Camera>(item));
                break;
            case Item::Kind::Light:
                visitor->visit(boost::static_pointer_cast<Light>(item));
                break;
        }
    }
}

/**
 **/
void Scene::deselect() noexcept {
    for (const boost::shared_ptr<Item>& item : items_) {
        item->selected(false);
    }
    deselectComponents();
}

/**
 **/
void Scene::deselectComponents() noexcept {
    for (const boost::shared_ptr<Item>& item : items_) {
        const boost::shared_ptr<Mesh> mesh = as<Mesh>(item);
        if (mesh) {
            mesh->geometry()->deselectComponents();
        }
    }
}

};  // namespace v3d::editor
