/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Transform.h"

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 **/
Transform::Transform(const std::vector<Change>& changes, const std::string& mode) :
    changes_(changes),
    mode_(mode) {
}

/**
 **/
void Transform::undo() {
    for (const Change& change : changes_) {
        if (change.item) {
            change.item->placement(change.before);
        }
    }
}

/**
 **/
void Transform::redo() {
    for (const Change& change : changes_) {
        if (change.item) {
            change.item->placement(change.after);
        }
    }
}

/**
 **/
std::string Transform::name() const {
    return mode_;
}

};  // namespace v3d::editor::command
