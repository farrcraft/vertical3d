/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/LineCanvas.h>

#include "SceneVisitor.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * Draws every mesh of a scene as a wireframe, onto one line canvas.
 *
 * A mesh is drawn through its own transform, so the canvas carries the placement rather
 * than the geometry, and each edge is drawn once - see ownsEdge().
 *
 * Wireframe is the only display mode the editor has - a shaded mode needs a triangle
 * primitive, which is not the line one. That is also why a selected face is drawn as
 * its boundary and a selected vertex as a small box: there is nothing to fill either
 * with.
 **/
class WireframeVisitor final : public SceneVisitor {
 public:
    /**
     * @param canvas where the segments are appended - nothing is cleared
     **/
    explicit WireframeVisitor(v3d::render::realtime::LineCanvas* canvas);

    /**
     * Append one mesh's wireframe.
     **/
    void visit(const boost::shared_ptr<v3d::brep::BRep>& mesh) override;

 private:
    /**
     * Whether either half of an edge is selected.
     **/
    static bool edgeSelected(const boost::shared_ptr<v3d::brep::BRep>& mesh, unsigned int edge);

    /**
     * A box at each selected vertex, sized against the mesh.
     **/
    void markers(const boost::shared_ptr<v3d::brep::BRep>& mesh);

    v3d::render::realtime::LineCanvas* canvas_;
    glm::vec4 wire_;
    glm::vec4 object_;
    glm::vec4 component_;
};

};  // namespace v3d::editor
