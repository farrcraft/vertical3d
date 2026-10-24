/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TopologyCommand.h"

#include <string>
#include <vector>

namespace v3d::editor {

/**
 **/
TopologyCommand::TopologyCommand(const std::vector<Change>& changes, const std::string& name) :
    changes_(changes),
    name_(name) {
}

/**
 **/
void TopologyCommand::undo() {
    for (const Change& change : changes_) {
        if (change.mesh) {
            change.mesh->restore(change.before);
            change.mesh->deselectComponents();
        }
    }
}

/**
 **/
void TopologyCommand::redo() {
    for (const Change& change : changes_) {
        if (change.mesh) {
            change.mesh->restore(change.after);
            change.mesh->deselectComponents();
        }
    }
}

/**
 **/
std::string TopologyCommand::name() const {
    return name_;
}

};  // namespace v3d::editor
