/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>
#include <api/render/realtime/canvas/Line.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Components.h>
#include <vertical3d/src/scene/Curve.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/SceneVisitor.h>

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * Draws a scene in lines, onto one line canvas: every mesh as a wireframe, every curve through
 * its evaluated points, and every camera, light and group as its glyph. Which kinds are drawn is
 * chosen when the visitor is made, and a group and a curve are always drawn.
 *
 * A mesh is drawn through its world matrix, so the canvas carries the placement, not the
 * geometry. Each edge is drawn once, by the half edge ownsEdge() picks. A glyph is drawn
 * through its world matrix without the scale, so it keeps its size. A selected item is drawn
 * in the selection's colour.
 *
 * A selected curve item can also show what shapes it: a small box at each control point, the
 * lines between a NURBS curve's control points, and a line from each Bezier point to each of its
 * handles with a box at the handle's end. A selected control point or handle is drawn in the
 * component colour.
 *
 * These are lines, drawn over the shaded pass. A selected face is drawn as its boundary, and a
 * selected vertex as a small box.
 **/
class WireframeVisitor final : public SceneVisitor {
 public:
    /**
     * Which kinds of item are drawn. A kind left out is skipped.
     **/
    struct Shown {
        bool meshes = true;
        bool cameras = true;
        bool lights = true;
        /**
         * Whether a selected curve item shows its control points.
         **/
        bool controls = false;
        /**
         * Whether a selected curve item that shows its control points shows its Bezier
         * handles as well.
         **/
        bool handles = false;
    };

    /**
     * Draw every kind.
     *
     * @param canvas where the segments are appended. Nothing is cleared
     **/
    explicit WireframeVisitor(v3d::render::realtime::canvas::Line* canvas);

    /**
     * Draw the kinds shown.
     *
     * @param components which parts of each mesh and curve to draw as selected, or null for none
     **/
    WireframeVisitor(v3d::render::realtime::canvas::Line* canvas, const Shown& shown,
        const Components* components = nullptr);

    /**
     * Append one mesh's wireframe.
     **/
    void visit(const boost::shared_ptr<Mesh>& item, const Placed& placed) override;

    /**
     * Append one camera's, light's or group's glyph.
     **/
    void visit(const boost::shared_ptr<Camera>& camera, const Placed& placed, const Camera::Lens& lens) override;
    void visit(const boost::shared_ptr<Light>& light, const Placed& placed, const Light::Emission& emission) override;
    void visit(const boost::shared_ptr<Group>& group, const Placed& placed) override;

    /**
     * Append one item's curves, through its world matrix.
     **/
    void visit(const boost::shared_ptr<Curve>& curve, const Placed& placed) override;

 private:
    /**
     * A box at each selected vertex, sized against the mesh.
     **/
    void markers(unsigned int item, const boost::shared_ptr<v3d::brep::BRep>& mesh);

    /**
     * A selected curve's control points, its NURBS hulls and its Bezier handles, as shown.
     **/
    void controls(const Curve& item);

    v3d::render::realtime::canvas::Line* canvas_;
    Shown shown_;
    const Components* components_;
    glm::vec4 wire_;
    glm::vec4 glyph_;
    glm::vec4 object_;
    glm::vec4 component_;
};

};  // namespace v3d::editor
