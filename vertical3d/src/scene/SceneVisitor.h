/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Camera.h"
#include "Light.h"
#include "Mesh.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * What walks a scene.
 *
 * There is one visit() for each kind of item, and every one is pure, so a kind added to the
 * scene does not compile until every visitor says what it does with it.
 **/
class SceneVisitor {
 public:
    virtual ~SceneVisitor() { }

    virtual void visit(const boost::shared_ptr<Mesh>& mesh) = 0;
    virtual void visit(const boost::shared_ptr<Camera>& camera) = 0;
    virtual void visit(const boost::shared_ptr<Light>& light) = 0;
};

};  // namespace v3d::editor
