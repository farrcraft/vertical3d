/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Topology.h"

#include <vertical3d/src/scene/Scene.h>

#include <string>
#include <vector>

namespace v3d::editor::command {

/**
 **/
Topology::Topology(const boost::shared_ptr<scene::Scene>& scene, const std::vector<Change>& changes,
    const std::string& name) :
    scene_(scene),
    changes_(changes),
    name_(name) {
}

/**
 **/
void Topology::undo() {
    for (const Change& change : changes_) {
        if (change.mesh) {
            change.mesh->restore(change.before);
            if (scene_) {
                scene_->deselectComponents(change.mesh.get());
            }
        }
    }
}

/**
 **/
void Topology::redo() {
    for (const Change& change : changes_) {
        if (change.mesh) {
            change.mesh->restore(change.after);
            if (scene_) {
                scene_->deselectComponents(change.mesh.get());
            }
        }
    }
}

/**
 **/
std::string Topology::name() const {
    return name_;
}

};  // namespace v3d::editor::command
