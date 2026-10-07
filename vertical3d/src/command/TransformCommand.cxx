/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TransformCommand.h"

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
TransformCommand::TransformCommand(const boost::shared_ptr<v3d::brep::BRep>& mesh,
    const Placement& before,
    const Placement& after,
    const std::string& mode) :
    mesh_(mesh),
    before_(before),
    after_(after),
    mode_(mode) {
}

/**
 **/
void TransformCommand::undo() {
    if (mesh_) {
        before_.applyTo(mesh_.get());
    }
}

/**
 **/
void TransformCommand::redo() {
    if (mesh_) {
        after_.applyTo(mesh_.get());
    }
}

/**
 **/
std::string TransformCommand::name() const {
    return mode_;
}

};  // namespace v3d::editor
