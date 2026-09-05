/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MeshTopology.h"

#include <cstddef>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

    /**
     **/
    std::vector<unsigned int> faceLoop(const boost::shared_ptr<v3d::brep::BRep>& mesh, unsigned int face) {
        std::vector<unsigned int> edges;
        if (!mesh) {
            return edges;
        }
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

    /**
     **/
    bool loopSegment(const boost::shared_ptr<v3d::brep::BRep>& mesh, const std::vector<unsigned int>& loop,
        std::size_t index, glm::vec3* from, glm::vec3* to) {
        if (!mesh || loop.empty() || index >= loop.size()) {
            return false;
        }

        v3d::brep::HalfEdge* edge = mesh->edge(loop[index]);
        v3d::brep::HalfEdge* before = mesh->edge(loop[(index + loop.size() - 1) % loop.size()]);
        if (edge == nullptr || before == nullptr) {
            return false;
        }

        v3d::brep::Vertex* start = mesh->vertex(static_cast<unsigned int>(before->vertex()));
        v3d::brep::Vertex* end = mesh->vertex(static_cast<unsigned int>(edge->vertex()));
        if (start == nullptr || end == nullptr) {
            return false;
        }

        if (from != nullptr) {
            *from = start->point();
        }
        if (to != nullptr) {
            *to = end->point();
        }
        return true;
    }

    /**
     **/
    bool ownsEdge(const boost::shared_ptr<v3d::brep::BRep>& mesh, unsigned int edge) {
        if (!mesh) {
            return false;
        }
        v3d::brep::HalfEdge* half = mesh->edge(edge);
        if (half == nullptr) {
            return false;
        }
        const uint64_t pair = half->pair();  // NOLINT(build/include_what_you_use) - the half edge, not std::pair
        return pair == v3d::brep::INVALID_ID || pair >= edge;
    }

};  // namespace v3d::editor
