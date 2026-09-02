/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "WireframeVisitor.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "MeshTopology.h"

#include "../../api/type/AABBox.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

    namespace {

        /**
         * An unselected mesh.
         **/
        const glm::vec4 wire(0.78f, 0.80f, 0.84f, 1.0f);

        /**
         * A mesh selected as a whole, which is object mode selection.
         **/
        const glm::vec4 object(0.35f, 0.72f, 1.0f, 1.0f);

        /**
         * A selected component: an edge, the boundary of a selected face, or the marker
         * drawn at a selected vertex.
         **/
        const glm::vec4 component(1.0f, 0.62f, 0.19f, 1.0f);

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
        for (std::size_t index = 0; index < faces; index++) {
            const unsigned int number = static_cast<unsigned int>(index);
            v3d::brep::Face* face = mesh->face(number);
            // a selected face is drawn as its boundary, there being no filled primitive to
            // shade it with
            const bool selected = face != nullptr && face->selected();
            const std::vector<unsigned int> loop = faceLoop(mesh, number);
            if (loop.size() < 2) {
                continue;
            }

            for (std::size_t entry = 0; entry < loop.size(); entry++) {
                const unsigned int current = loop[entry];
                if (!ownsEdge(mesh, current)) {
                    continue;
                }

                glm::vec3 from, to;
                if (!loopSegment(mesh, loop, entry, &from, &to)) {
                    continue;
                }

                canvas_->line(from, to, selected || edgeSelected(mesh, current) ? component_ : base);
            }
        }

        markers(mesh);

        canvas_->pop();
    }

    /**
     **/
    bool WireframeVisitor::edgeSelected(const boost::shared_ptr<v3d::brep::BRep>& mesh, unsigned int edge) const {
        v3d::brep::HalfEdge* half = mesh->edge(edge);
        if (half == nullptr) {
            return false;
        }
        if (half->selected()) {
            return true;
        }
        // the two halves are one edge to a selection, so either being selected colours
        // the segment
        const uint64_t pair = half->pair();  // NOLINT(build/include_what_you_use) - the half edge, not std::pair
        if (pair == v3d::brep::INVALID_ID) {
            return false;
        }
        v3d::brep::HalfEdge* other = mesh->edge(static_cast<unsigned int>(pair));
        return other != nullptr && other->selected();
    }

    /**
     **/
    void WireframeVisitor::markers(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
        const std::size_t count = mesh->vertexCount();
        if (count == 0) {
            return;
        }

        float size = 0.0f;
        const v3d::type::AABBox bound = mesh->bound();
        const glm::vec3 extent = bound.max() - bound.min();
        size = std::max(std::max(extent.x, extent.y), extent.z) * markerScale;
        if (size <= 0.0f) {
            // a flat or degenerate mesh still gets a marker that can be seen
            size = markerScale;
        }

        const glm::vec3 corner(size, size, size);
        for (std::size_t index = 0; index < count; index++) {
            v3d::brep::Vertex* vertex = mesh->vertex(static_cast<unsigned int>(index));
            if (vertex == nullptr || !vertex->selected()) {
                continue;
            }
            const glm::vec3 point = vertex->point();
            canvas_->box(point - corner, point + corner, component_);
        }
    }

};  // namespace v3d::editor
