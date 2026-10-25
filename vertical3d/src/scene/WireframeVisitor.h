/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>
#include <api/render/realtime/canvas/Line.h>

#include "Camera.h"
#include "Light.h"
#include "Mesh.h"
#include "SceneVisitor.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * Draws a scene in lines, onto one line canvas: every mesh as a wireframe, and every camera
 * and light as its glyph. Which kinds are drawn is chosen when the visitor is made.
 *
 * A mesh is drawn through its own transform, so the canvas carries the placement, not the
 * geometry. Each edge is drawn once, by the half edge ownsEdge() picks. A glyph is drawn
 * through its item's translation and rotation, so it keeps its size. A selected item is drawn
 * in the selection's colour.
 *
 * Wireframe is the editor's only display mode, because a shaded mode needs a triangle
 * primitive and this draws lines. For the same reason a selected face is drawn as its
 * boundary, and a selected vertex as a small box.
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
    };

    /**
     * Draw every kind.
     *
     * @param canvas where the segments are appended. Nothing is cleared
     **/
    explicit WireframeVisitor(v3d::render::realtime::canvas::Line* canvas);

    /**
     * Draw the kinds shown.
     **/
    WireframeVisitor(v3d::render::realtime::canvas::Line* canvas, const Shown& shown);

    /**
     * Append one mesh's wireframe.
     **/
    void visit(const boost::shared_ptr<Mesh>& item) override;

    /**
     * Append one camera's or light's glyph.
     **/
    void visit(const boost::shared_ptr<Camera>& camera) override;
    void visit(const boost::shared_ptr<Light>& light) override;

 private:
    /**
     * A box at each selected vertex, sized against the mesh.
     **/
    void markers(const boost::shared_ptr<v3d::brep::BRep>& mesh);

    v3d::render::realtime::canvas::Line* canvas_;
    Shown shown_;
    glm::vec4 wire_;
    glm::vec4 glyph_;
    glm::vec4 object_;
    glm::vec4 component_;
};

};  // namespace v3d::editor
