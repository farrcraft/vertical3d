/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Camera.h"
#include "Curve.h"
#include "Evaluation.h"
#include "Group.h"
#include "Item.h"
#include "Light.h"
#include "Mesh.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * What walks a scene.
 *
 * There is one visit() for each kind of item, and every one is pure, so a kind added to the
 * scene does not compile until every visitor says what it does with it. Each is given where
 * the item is, and a camera's lens or a light's emission, from one evaluation of the scene
 * made for the walk. A visitor reads those, not the item's own values, which a curve may
 * override.
 *
 * The walk is depth first: an item is visited, then everything beneath it, and then leave()
 * is called for it. Siblings come in the scene's order, and so do the items with no parent.
 **/
class SceneVisitor {
 public:
    virtual ~SceneVisitor() { }

    virtual void visit(const boost::shared_ptr<Mesh>& mesh, const Placed& placed) = 0;
    virtual void visit(const boost::shared_ptr<Camera>& camera, const Placed& placed, const Camera::Lens& lens) = 0;
    virtual void visit(const boost::shared_ptr<Light>& light, const Placed& placed, const Light::Emission& emission) = 0;
    virtual void visit(const boost::shared_ptr<Group>& group, const Placed& placed) = 0;
    virtual void visit(const boost::shared_ptr<Curve>& curve, const Placed& placed) = 0;

    /**
     * Called once everything beneath an item has been visited. A visitor that does not care
     * about the hierarchy leaves this alone.
     **/
    virtual void leave(const boost::shared_ptr<Item>& item) {
        static_cast<void>(item);
    }
};

};  // namespace v3d::editor
