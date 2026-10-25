/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CreateCommand.h"

#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
CreateCommand::CreateCommand(const boost::shared_ptr<Scene>& scene,
    const boost::shared_ptr<Item>& item,
    const std::string& kind) :
    scene_(scene),
    item_(item),
    kind_(kind) {
}

/**
 **/
void CreateCommand::redo() {
    if (!scene_ || !item_) {
        return;
    }
    // a new item becomes the selection, so the transform tools act on it
    scene_->deselect();
    item_->selected(true);
    scene_->add(item_);
}

/**
 **/
void CreateCommand::undo() {
    if (!scene_ || !item_) {
        return;
    }
    // the flags live on the item rather than in the scene, so they are cleared as the item
    // leaves. Any other selected item stays selected
    item_->selected(false);
    const boost::shared_ptr<Mesh> mesh = as<Mesh>(item_);
    if (mesh) {
        mesh->geometry()->deselectComponents();
    }
    scene_->remove(item_->id());
}

/**
 **/
std::string CreateCommand::name() const {
    return "create " + kind_;
}

};  // namespace v3d::editor
