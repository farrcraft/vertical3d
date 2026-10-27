/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "WireframeVisitor.h"

#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Topology.h>
#include <api/brep/Vertex.h>
#include <api/render/realtime/canvas/Line.h>
#include <api/type/geometry/AABBox.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

#include "Camera.h"
#include "Evaluation.h"
#include "Glyphs.h"
#include "Group.h"
#include "Light.h"
#include "Mesh.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

namespace {

/**
 * An unselected mesh.
 **/
constexpr glm::vec4 wire(0.78F, 0.80F, 0.84F, 1.0F);

/**
 * An unselected camera or light, apart from the meshes around it.
 **/
constexpr glm::vec4 glyph(0.95F, 0.82F, 0.42F, 1.0F);

/**
 * An item selected as a whole, which is object mode selection.
 **/
constexpr glm::vec4 object(0.35F, 0.72F, 1.0F, 1.0F);

/**
 * A selected component: an edge, the boundary of a selected face, or the marker
 * drawn at a selected vertex.
 **/
constexpr glm::vec4 component(1.0F, 0.62F, 0.19F, 1.0F);

/**
 * How big a selected vertex's marker is, as a fraction of the mesh's largest
 * dimension. A line canvas is world space, so the marker cannot be sized in pixels
 * and is sized against the mesh instead.
 **/
constexpr float markerScale = 0.04F;

};  // namespace

/**
 **/
WireframeVisitor::WireframeVisitor(v3d::render::realtime::canvas::Line* canvas) :
    WireframeVisitor(canvas, Shown{}) {
}

/**
 **/
WireframeVisitor::WireframeVisitor(v3d::render::realtime::canvas::Line* canvas, const Shown& shown) :
    canvas_(canvas),
    shown_(shown),
    wire_(wire),
    glyph_(glyph),
    object_(object),
    component_(component) {
}

/**
 **/
void WireframeVisitor::visit(const boost::shared_ptr<Mesh>& item, const Placed& placed) {
    if (canvas_ == nullptr || !item || !shown_.meshes) {
        return;
    }

    const glm::vec4& base = item->selected() ? object_ : wire_;
    const boost::shared_ptr<v3d::brep::BRep>& mesh = item->geometry();

    canvas_->push();
    canvas_->transform(placed.world);

    const std::size_t faces = mesh->faceCount();
    // a selected face is drawn as its boundary, there being no filled primitive to shade it
    // with. An edge is drawn once, by one of its two halves, so it is highlighted when the face
    // on either side of it is selected
    const auto faceSelected = [&mesh](v3d::brep::Index edge) {
        const v3d::brep::HalfEdge* half = mesh->edge(edge);
        if (half == nullptr) {
            return false;
        }
        const v3d::brep::Face* face = mesh->face(half->face());
        return face != nullptr && face->selected();
    };

    for (v3d::brep::Index number = 0; number < faces; number++) {
        const std::vector<v3d::brep::Index> loop = v3d::brep::faceLoop(*mesh, number);
        if (loop.size() < 2) {
            continue;
        }

        for (std::size_t entry = 0; entry < loop.size(); entry++) {
            const v3d::brep::Index current = loop[entry];
            if (!v3d::brep::ownsEdge(*mesh, current)) {
                continue;
            }

            const std::optional<v3d::brep::Segment> segment = v3d::brep::loopSegment(*mesh, loop, entry);
            if (!segment) {
                continue;
            }
            const glm::vec3& from = segment->from;
            const glm::vec3& to = segment->to;

            const v3d::brep::Index pair = mesh->edge(current)->pair();
            const bool selected = faceSelected(current) || (pair != v3d::brep::INVALID_ID && faceSelected(pair)) ||
                v3d::brep::edgeSelected(*mesh, current);
            canvas_->line(from, to, selected ? component_ : base);
        }
    }

    markers(mesh);

    canvas_->pop();
}

/**
 **/
void WireframeVisitor::visit(const boost::shared_ptr<Camera>& camera, const Placed& placed, const Camera::Lens& lens) {
    if (canvas_ == nullptr || !camera || !shown_.cameras) {
        return;
    }
    canvas_->push();
    canvas_->transform(glyphPlacement(placed.world));
    drawCamera(lens, camera->selected() ? object_ : glyph_, canvas_);
    canvas_->pop();
}

/**
 **/
void WireframeVisitor::visit(const boost::shared_ptr<Light>& light, const Placed& placed, const Light::Emission& emission) {
    if (canvas_ == nullptr || !light || !shown_.lights) {
        return;
    }
    canvas_->push();
    canvas_->transform(glyphPlacement(placed.world));
    drawLight(emission, light->selected() ? object_ : glyph_, canvas_);
    canvas_->pop();
}

/**
 **/
void WireframeVisitor::visit(const boost::shared_ptr<Group>& group, const Placed& placed) {
    if (canvas_ == nullptr || !group) {
        return;
    }
    canvas_->push();
    canvas_->transform(glyphPlacement(placed.world));
    drawGroup(group->selected() ? object_ : glyph_, canvas_);
    canvas_->pop();
}

/**
 **/
void WireframeVisitor::markers(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    const std::size_t count = mesh->vertexCount();
    if (count == 0) {
        return;
    }

    float size = 0.0F;
    const v3d::type::geometry::AABBox bound = mesh->bound();
    const glm::vec3 extent = bound.max() - bound.min();
    size = std::max(std::max(extent.x, extent.y), extent.z) * markerScale;
    if (size <= 0.0F) {
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
