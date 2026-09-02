/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scene.h"

#include <algorithm>
#include <cstddef>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

    /**
     **/
    unsigned int Scene::add(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
        if (!mesh) {
            return 0;
        }
        meshes_.push_back(mesh);
        return mesh->id();
    }

    /**
     **/
    bool Scene::remove(unsigned int id) {
        auto it = std::find_if(meshes_.begin(), meshes_.end(),
            [id](const boost::shared_ptr<v3d::brep::BRep>& mesh) { return mesh->id() == id; });
        if (it == meshes_.end()) {
            return false;
        }
        meshes_.erase(it);
        return true;
    }

    /**
     **/
    boost::shared_ptr<v3d::brep::BRep> Scene::mesh(unsigned int id) const {
        auto it = std::find_if(meshes_.begin(), meshes_.end(),
            [id](const boost::shared_ptr<v3d::brep::BRep>& mesh) { return mesh->id() == id; });
        if (it == meshes_.end()) {
            return boost::shared_ptr<v3d::brep::BRep>();
        }
        return *it;
    }

    /**
     **/
    std::size_t Scene::count() const noexcept {
        return meshes_.size();
    }

    /**
     **/
    void Scene::clear() noexcept {
        meshes_.clear();
    }

    /**
     **/
    void Scene::accept(SceneVisitor* visitor) const {
        if (visitor == nullptr) {
            return;
        }
        for (const boost::shared_ptr<v3d::brep::BRep>& mesh : meshes_) {
            visitor->visit(mesh);
        }
    }

    /**
     **/
    void Scene::deselect() noexcept {
        for (const boost::shared_ptr<v3d::brep::BRep>& mesh : meshes_) {
            mesh->selected(false);
            mesh->deselectComponents();
        }
    }

    /**
     **/
    void Scene::deselectComponents() noexcept {
        for (const boost::shared_ptr<v3d::brep::BRep>& mesh : meshes_) {
            mesh->deselectComponents();
        }
    }

};  // namespace v3d::editor
