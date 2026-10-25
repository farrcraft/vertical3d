/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TransformCommand.h"

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
TransformCommand::TransformCommand(const std::vector<Change>& changes, const std::string& mode) :
    changes_(changes),
    mode_(mode) {
}

/**
 **/
void TransformCommand::undo() {
    for (const Change& change : changes_) {
        if (change.item) {
            change.before.applyTo(change.item.get());
        }
    }
}

/**
 **/
void TransformCommand::redo() {
    for (const Change& change : changes_) {
        if (change.item) {
            change.after.applyTo(change.item.get());
        }
    }
}

/**
 **/
std::string TransformCommand::name() const {
    return mode_;
}

};  // namespace v3d::editor
