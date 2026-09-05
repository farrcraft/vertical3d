/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CreateCommand.h"

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
CreateCommand::CreateCommand(const boost::shared_ptr<Scene>& scene,
    const boost::shared_ptr<v3d::brep::BRep>& mesh,
    const std::string& kind) :
    scene_(scene),
    mesh_(mesh),
    kind_(kind) {
}

/**
 **/
void CreateCommand::redo() {
    if (!scene_ || !mesh_) {
        return;
    }
    // a new mesh is the selected one, which is what the transform tools act on
    scene_->deselect();
    mesh_->selected(true);
    scene_->add(mesh_);
}

/**
 **/
void CreateCommand::undo() {
    if (!scene_ || !mesh_) {
        return;
    }
    // the flag lives on the mesh rather than in the scene, so a mesh taken out while
    // selected would come back to a scene that already has a selection
    mesh_->selected(false);
    mesh_->deselectComponents();
    scene_->remove(mesh_->id());
}

/**
 **/
std::string CreateCommand::name() const {
    return "create " + kind_;
}

};  // namespace v3d::editor
