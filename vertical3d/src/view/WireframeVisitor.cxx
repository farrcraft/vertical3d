/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "WireframeVisitor.h"

#include <api/brep/BRep.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Topology.h>
#include <api/brep/Vertex.h>
#include <api/curve/Curve.h>
#include <api/curve/Curves.h>
#include <api/render/realtime/canvas/Line.h>
#include <api/type/geometry/AABBox.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Components.h>
#include <vertical3d/src/scene/Curve.h>
#include <vertical3d/src/scene/CurveParts.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "Glyphs.h"

#include <boost/shared_ptr.hpp>
#include <glm/common.hpp>

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

/**
 * How big a curve's control point or handle marker is, against the curve's largest dimension.
 * It is smaller than a vertex's, because a curve's points lie closer together than a mesh's.
 **/
constexpr float controlScale = 0.015F;

/**
 * A marker's half size for geometry this large across, kept visible when the geometry is flat.
 **/
float markerSize(const glm::vec3& extent, float scale) {
    const float size = std::max(std::max(extent.x, extent.y), extent.z) * scale;
    return size > 0.0F ? size : scale;
}

};  // namespace

/**
 **/
WireframeVisitor::WireframeVisitor(v3d::render::realtime::canvas::Line* canvas) :
    WireframeVisitor(canvas, Shown{}) {
}

/**
 **/
WireframeVisitor::WireframeVisitor(v3d::render::realtime::canvas::Line* canvas, const Shown& shown,
    const Components* components) :
    canvas_(canvas),
    shown_(shown),
    components_(components),
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
    const unsigned int id = item->id();
    const auto faceSelected = [this, &mesh, id](v3d::brep::Index edge) {
        const v3d::brep::HalfEdge* half = mesh->edge(edge);
        return half != nullptr && components_ != nullptr && components_->selected(id, Component::Face, half->face());
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
                (components_ != nullptr && components_->edge(id, *mesh, current));
            canvas_->line(from, to, selected ? component_ : base);
        }
    }

    markers(id, mesh);

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
void WireframeVisitor::visit(const boost::shared_ptr<Curve>& curve, const Placed& placed) {
    if (canvas_ == nullptr || !curve) {
        return;
    }
    canvas_->push();
    canvas_->transform(placed.world);
    drawCurves(*curve->geometry(), curve->selected() ? object_ : wire_, canvas_);
    if (shown_.controls && curve->selected()) {
        controls(*curve);
    }
    canvas_->pop();
}

/**
 **/
void WireframeVisitor::controls(const Curve& item) {
    const v3d::curve::Curves& curves = *item.geometry();
    const std::vector<std::pair<v3d::brep::Index, glm::vec3>> points = controlPoints(curves);
    if (points.empty()) {
        return;
    }
    const std::vector<std::pair<v3d::brep::Index, glm::vec3>> handles =
        shown_.handles ? handlePoints(curves) : std::vector<std::pair<v3d::brep::Index, glm::vec3>>{};

    glm::vec3 low = points.front().second;
    glm::vec3 high = low;
    for (const auto& [part, point] : points) {
        low = glm::min(low, point);
        high = glm::max(high, point);
    }
    const float size = markerSize(high - low, controlScale);
    const glm::vec3 corner(size, size, size);
    const unsigned int id = item.id();
    const auto selected = [this, id](Component kind, v3d::brep::Index part) {
        return components_ != nullptr && components_->selected(id, kind, part);
    };

    for (const auto& [part, point] : points) {
        canvas_->box(point - corner, point + corner, selected(Component::Vertex, part) ? component_ : wire_);
    }

    // a NURBS curve does not pass through its points, so the hull between them shows how they
    // pull on it
    for (std::size_t index = 0; index < curves.size(); index++) {
        const v3d::curve::Curve& curve = curves.curve(index);
        const auto* nurbs = std::get_if<v3d::curve::Nurbs>(&curve.shape);
        if (nurbs == nullptr) {
            continue;
        }
        for (std::size_t each = 0; each + 1 < nurbs->points.size(); each++) {
            canvas_->line(nurbs->points[each], nurbs->points[each + 1], glyph_);
        }
        if (curve.closed && nurbs->points.size() > 2) {
            canvas_->line(nurbs->points.back(), nurbs->points.front(), glyph_);
        }
    }

    // handlePoints() gives the left and right handle of each point in turn, so a handle's point
    // is the control point its part number names
    for (const auto& [part, handle] : handles) {
        const std::optional<CurveHandle> at = handleAt(curves, part);
        if (!at) {
            continue;
        }
        const glm::vec3 point = controlPoint(curves.curve(at->at.curve), at->at.point);
        const glm::vec4& colour = selected(Component::Handle, part) ? component_ : glyph_;
        canvas_->line(point, handle, colour);
        canvas_->box(handle - corner, handle + corner, colour);
    }
}

/**
 **/
void WireframeVisitor::markers(unsigned int item, const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    const std::size_t count = mesh->vertexCount();
    if (count == 0 || components_ == nullptr || !components_->any(item)) {
        return;
    }

    // a flat or degenerate mesh still gets a marker that can be seen
    const v3d::type::geometry::AABBox bound = mesh->bound();
    const float size = markerSize(bound.max() - bound.min(), markerScale);

    const glm::vec3 corner(size, size, size);
    for (const v3d::brep::Index index : components_->of(item, Component::Vertex)) {
        const v3d::brep::Vertex* vertex = mesh->vertex(index);
        if (vertex == nullptr) {
            continue;
        }
        const glm::vec3 point = vertex->point();
        canvas_->box(point - corner, point + corner, component_);
    }
}

};  // namespace v3d::editor
