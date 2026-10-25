/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/canvas/Line.h>

#include <api/brep/BRep.h>

#include "Camera.h"
#include "Light.h"
#include "Mesh.h"
#include "SceneVisitor.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * Draws every mesh of a scene as a wireframe, onto one line canvas. Cameras and lights are not
 * drawn.
 *
 * A mesh is drawn through its own transform, so the canvas carries the placement, not the
 * geometry. Each edge is drawn once, by the half edge ownsEdge() picks.
 *
 * Wireframe is the editor's only display mode, because a shaded mode needs a triangle
 * primitive and this draws lines. For the same reason a selected face is drawn as its
 * boundary, and a selected vertex as a small box.
 **/
class WireframeVisitor final : public SceneVisitor {
 public:
    /**
     * @param canvas where the segments are appended. Nothing is cleared
     **/
    explicit WireframeVisitor(v3d::render::realtime::canvas::Line* canvas);

    /**
     * Append one mesh's wireframe.
     **/
    void visit(const boost::shared_ptr<Mesh>& item) override;

    /**
     * Cameras and lights are not drawn.
     **/
    void visit(const boost::shared_ptr<Camera>& camera) override;
    void visit(const boost::shared_ptr<Light>& light) override;

 private:
    /**
     * A box at each selected vertex, sized against the mesh.
     **/
    void markers(const boost::shared_ptr<v3d::brep::BRep>& mesh);

    v3d::render::realtime::canvas::Line* canvas_;
    glm::vec4 wire_;
    glm::vec4 object_;
    glm::vec4 component_;
};

};  // namespace v3d::editor
