/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TopologyCommand.h"

#include <vertical3d/src/scene/Scene.h>

#include <string>
#include <vector>

namespace v3d::editor {

/**
 **/
TopologyCommand::TopologyCommand(const boost::shared_ptr<Scene>& scene, const std::vector<Change>& changes,
    const std::string& name) :
    scene_(scene),
    changes_(changes),
    name_(name) {
}

/**
 **/
void TopologyCommand::undo() {
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
void TopologyCommand::redo() {
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
std::string TopologyCommand::name() const {
    return name_;
}

};  // namespace v3d::editor
