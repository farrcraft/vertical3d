/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../../../api/brep/BRep.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * What walks a scene.
 *
 * A scene holds meshes and nothing else so far, so there is one visit(); a visitor that
 * wants only some of what a scene holds is what the interface grows into when lights,
 * cameras and groups become nodes of their own.
 **/
class SceneVisitor {
 public:
    virtual ~SceneVisitor() { }

    virtual void visit(const boost::shared_ptr<v3d::brep::BRep>& mesh) = 0;
};

};  // namespace v3d::editor
