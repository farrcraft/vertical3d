/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/Curve.h>
#include <vertical3d/src/scene/item/Group.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Light.h>
#include <vertical3d/src/scene/item/Mesh.h>

#include "Evaluation.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::scene {

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
class Visitor {
 public:
    virtual ~Visitor() { }

    virtual void visit(const boost::shared_ptr<item::Mesh>& mesh, const Placed& placed) = 0;
    virtual void visit(const boost::shared_ptr<item::Camera>& camera, const Placed& placed, const item::Camera::Lens& lens) = 0;
    virtual void visit(const boost::shared_ptr<item::Light>& light, const Placed& placed, const item::Light::Emission& emission) = 0;
    virtual void visit(const boost::shared_ptr<item::Group>& group, const Placed& placed) = 0;
    virtual void visit(const boost::shared_ptr<item::Curve>& curve, const Placed& placed) = 0;

    /**
     * Called once everything beneath an item has been visited. A visitor that does not care
     * about the hierarchy leaves this alone.
     **/
    virtual void leave(const boost::shared_ptr<item::Item>& item) {
        static_cast<void>(item);
    }
};

};  // namespace v3d::editor::scene
