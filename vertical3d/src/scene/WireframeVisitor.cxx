/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "WireframeVisitor.h"

#include <api/brep/Topology.h>
#include <api/type/geometry/AABBox.h>

#include <algorithm>
#include <cstddef>
#include <vector>


#include <boost/shared_ptr.hpp>

namespace v3d::editor {

namespace {

/**
 * An unselected mesh.
 **/
constexpr glm::vec4 wire(0.78f, 0.80f, 0.84f, 1.0f);

/**
 * A mesh selected as a whole, which is object mode selection.
 **/
constexpr glm::vec4 object(0.35f, 0.72f, 1.0f, 1.0f);

/**
 * A selected component: an edge, the boundary of a selected face, or the marker
 * drawn at a selected vertex.
 **/
constexpr glm::vec4 component(1.0f, 0.62f, 0.19f, 1.0f);

/**
 * How big a selected vertex's marker is, as a fraction of the mesh's largest
 * dimension. A line canvas is world space, so the marker cannot be sized in pixels
 * and is sized against the mesh instead.
 **/
constexpr float markerScale = 0.04f;

};  // namespace

/**
 **/
WireframeVisitor::WireframeVisitor(v3d::render::realtime::LineCanvas* canvas) :
    canvas_(canvas),
    wire_(wire),
    object_(object),
    component_(component) {
}

/**
 **/
void WireframeVisitor::visit(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    if (canvas_ == nullptr || !mesh) {
        return;
    }

    const glm::vec4& base = mesh->selected() ? object_ : wire_;

    canvas_->push();
    canvas_->transform(mesh->matrix());

    const std::size_t faces = mesh->faceCount();
    for (v3d::brep::Index number = 0; number < faces; number++) {
        const v3d::brep::Face* face = mesh->face(number);
        // a selected face is drawn as its boundary, there being no filled primitive to
        // shade it with
        const bool selected = face != nullptr && face->selected();
        const std::vector<v3d::brep::Index> loop = v3d::brep::faceLoop(*mesh, number);
        if (loop.size() < 2) {
            continue;
        }

        for (std::size_t entry = 0; entry < loop.size(); entry++) {
            const v3d::brep::Index current = loop[entry];
            if (!v3d::brep::ownsEdge(*mesh, current)) {
                continue;
            }

            glm::vec3 from;
            glm::vec3 to;
            if (!v3d::brep::loopSegment(*mesh, loop, entry, &from, &to)) {
                continue;
            }

            canvas_->line(from, to, selected || v3d::brep::edgeSelected(*mesh, current) ? component_ : base);
        }
    }

    markers(mesh);

    canvas_->pop();
}

/**
 **/
void WireframeVisitor::markers(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    const std::size_t count = mesh->vertexCount();
    if (count == 0) {
        return;
    }

    float size = 0.0f;
    const v3d::type::geometry::AABBox bound = mesh->bound();
    const glm::vec3 extent = bound.max() - bound.min();
    size = std::max(std::max(extent.x, extent.y), extent.z) * markerScale;
    if (size <= 0.0f) {
        // a flat or degenerate mesh still gets a marker that can be seen
        size = markerScale;
    }

    const glm::vec3 corner(size, size, size);
    for (v3d::brep::Index index = 0; index < count; index++) {
        const v3d::brep::Vertex* vertex = mesh->vertex(index);
        if (vertex == nullptr || !vertex->selected()) {
            continue;
        }
        const glm::vec3 point = vertex->point();
        canvas_->box(point - corner, point + corner, component_);
    }
}

};  // namespace v3d::editor
