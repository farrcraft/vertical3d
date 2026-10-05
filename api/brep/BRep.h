/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/dag/Node.h>
#include <api/dag/Transform.h>
#include <api/type/geometry/AABBox.h>

#include <string>
#include <vector>

#include "Vertex.h"
#include "HalfEdge.h"
#include "Face.h"

#include <boost/shared_ptr.hpp>

namespace v3d::brep {

/**
 * A boundary representation mesh.
 *
 * It is a dag::Node, so it has an id the selection model keys on, and a dag::Transform,
 * so its geometry is described about its own origin and placed by matrix(). The three
 * manipulators write through the transform rather than through the vertices.
 **/
class BRep : public v3d::dag::Node, public v3d::dag::Transform {
 public:
        BRep();
        ~BRep();

        static const Index INVALID_ID;

        /**
         * A component by index, or null when the mesh holds no such one. Walking a face is
         * faceLoop()'s - see Topology.h.
         **/
        HalfEdge * edge(Index e);
        Face * face(Index f);
        Vertex * vertex(Index vert);
        const HalfEdge * edge(Index e) const;
        const Face * face(Index f) const;
        const Vertex * vertex(Index vert) const;

        /**
         * Whether every reference the mesh holds is to something it holds: a half edge's
         * vertex always, and its face, pair and next unless they are INVALID_ID; a face's
         * edge always. A next chain that does not close is allowed, since an unfinished
         * modelling operation leaves one, and faceLoop() ends it.
         *
         * @param problem what is wrong, for a caller to report, when anything is
         **/
        bool validate(std::string * problem) const;

        v3d::type::geometry::AABBox bound(void) const;

        /**
         * Whether the mesh as a whole is selected, which is object mode selection.
         * Component selection is the flag each Vertex, HalfEdge and Face carries.
         **/
        bool selected(void) const noexcept;
        void selected(bool sel) noexcept;

        /**
         * Clear the selection flag of every vertex, edge and face, leaving the mesh's
         * own flag alone - a select mask change deselects components, not objects.
         **/
        void deselectComponents(void) noexcept;

        void addFace(const std::vector<glm::vec3> & vertices, const glm::vec3 & normal);
        void addEdge(const glm::vec3 & point);

        size_t vertexCount(void) const;
        size_t edgeCount(void) const;
        size_t faceCount(void) const;

        Index addVertex(const Vertex & v);
        Index addEdge(const HalfEdge & e);
        Index addFace(const Face & f);

 protected:
        Index addVertex(const glm::vec3 & v);
        Index addEdge(Index vertex);
        Index findPair(Index edge, Index prevEdge);

 private:
        std::vector<Vertex> vertices_;
        std::vector<Face> faces_;
        std::vector<HalfEdge> edges_;
        bool selected_;
};


};  // namespace v3d::brep
