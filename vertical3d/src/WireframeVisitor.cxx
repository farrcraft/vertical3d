/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "WireframeVisitor.h"

#include <cstddef>
#include <vector>

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
         * A selected edge, or one whose pair is selected - the two halves are one edge to
         * anybody selecting it.
         **/
        const glm::vec4 component(1.0f, 0.62f, 0.19f, 1.0f);

        /**
         * The half edges of one face, in order.
         *
         * Bounded by the mesh's edge count, so a next chain that does not close - which is
         * what an unfinished modelling operation leaves behind - ends the walk rather than
         * spinning.
         **/
        std::vector<unsigned int> loop(const boost::shared_ptr<v3d::brep::BRep>& mesh, unsigned int face) {
            std::vector<unsigned int> edges;
            v3d::brep::Face* start = mesh->face(face);
            if (start == nullptr) {
                return edges;
            }
            unsigned int current = start->edge();
            while (edges.size() <= mesh->edgeCount()) {
                v3d::brep::HalfEdge* edge = mesh->edge(current);
                if (edge == nullptr) {
                    break;
                }
                edges.push_back(current);
                const uint64_t next = edge->next();
                if (next == v3d::brep::INVALID_ID || next == start->edge()) {
                    break;
                }
                current = static_cast<unsigned int>(next);
            }
            return edges;
        }

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
        for (std::size_t face = 0; face < faces; face++) {
            const std::vector<unsigned int> edges = loop(mesh, static_cast<unsigned int>(face));
            if (edges.size() < 2) {
                continue;
            }

            for (std::size_t index = 0; index < edges.size(); index++) {
                const unsigned int current = edges[index];
                v3d::brep::HalfEdge* edge = mesh->edge(current);
                if (edge == nullptr) {
                    continue;
                }

                const uint64_t pair = edge->pair();
                // a half edge and its pair are the same segment seen from the two faces
                // that share it, so the lower numbered of the two draws it
                if (pair != v3d::brep::INVALID_ID && pair < current) {  // NOLINT(build/include_what_you_use) - the half edge, not std::pair
                    continue;
                }

                // a half edge names the vertex it ends at, so its segment starts where the
                // one before it in the loop ended
                const unsigned int previous = edges[(index + edges.size() - 1) % edges.size()];
                v3d::brep::HalfEdge* before = mesh->edge(previous);
                if (before == nullptr) {
                    continue;
                }

                v3d::brep::Vertex* from = mesh->vertex(static_cast<unsigned int>(before->vertex()));
                v3d::brep::Vertex* to = mesh->vertex(static_cast<unsigned int>(edge->vertex()));
                if (from == nullptr || to == nullptr) {
                    continue;
                }

                bool selected = edge->selected();
                if (!selected && pair != v3d::brep::INVALID_ID) {
                    v3d::brep::HalfEdge* other = mesh->edge(static_cast<unsigned int>(pair));
                    selected = other != nullptr && other->selected();
                }

                canvas_->line(from->point(), to->point(), selected ? component_ : base);
            }
        }

        canvas_->pop();
    }

};  // namespace v3d::editor
