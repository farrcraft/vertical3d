/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ImportCommand.h"

#include <vertical3d/src/scene/Import.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <cstddef>
#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
ImportCommand::ImportCommand(const boost::shared_ptr<Scene>& scene, const Import& imported, const std::string& file) :
    scene_(scene),
    imported_(imported),
    file_(file) {
}

/**
 **/
void ImportCommand::redo() {
    if (!scene_) {
        return;
    }
    scene_->deselect();
    for (std::size_t index = 0; index < imported_.items.size(); index++) {
        const boost::shared_ptr<Item>& item = imported_.items[index];
        scene_->add(item);
        item->selected(!imported_.parents[index]);
        if (imported_.parents[index]) {
            scene_->parent(item->id(), imported_.items[*imported_.parents[index]]->id());
        }
        for (const auto& [path, curve] : imported_.curves[index]) {
            scene_->curve(item->id(), path, curve);
        }
    }
}

/**
 **/
void ImportCommand::undo() {
    if (!scene_) {
        return;
    }
    // removing an item at the top removes everything beneath it
    for (std::size_t index = 0; index < imported_.items.size(); index++) {
        if (!imported_.parents[index]) {
            imported_.items[index]->selected(false);
            scene_->remove(imported_.items[index]->id());
        }
    }
}

/**
 **/
std::string ImportCommand::name() const {
    return "import " + file_;
}

};  // namespace v3d::editor
