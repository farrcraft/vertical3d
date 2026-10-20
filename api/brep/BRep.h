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
 * It is a dag::Node, so it has an id the selection model keys on. It is a dag::Transform, so
 * its geometry is described about its own origin and placed by matrix(). A manipulator moves
 * the mesh through the transform, not through the vertices.
 **/
class BRep : public v3d::dag::Node, public v3d::dag::Transform {
 public:
        BRep();
        ~BRep();

        static const Index INVALID_ID;

        /**
         * A component by index, or null when the mesh holds no such one. faceLoop() in
         * Topology.h lists the half edges of a face.
         **/
        HalfEdge * edge(Index index);
        Face * face(Index index);
        Vertex * vertex(Index vert);
        const HalfEdge * edge(Index index) const;
        const Face * face(Index index) const;
        const Vertex * vertex(Index vert) const;

        /**
         * Whether every reference the mesh holds is to something it holds. A half edge's
         * vertex and a face's edge must always be valid. A half edge's face, pair and next
         * may also be INVALID_ID. A next chain that does not close is allowed: an unfinished
         * modelling operation leaves one, and faceLoop() stops on it.
         *
         * @param problem what is wrong, for a caller to report, when anything is
         **/
        bool validate(std::string * problem) const;

        /**
         * Whether the mesh is sound enough for an operation that changes its topology to be
         * checked against. Everything validate() checks, and also: every pair is mutual,
         * every face's next chain closes, every half edge in a face's loop names that face,
         * and no two half edges of one face run between the same two vertices. An edge with
         * no pair is allowed, because an open mesh has a boundary.
         *
         * @param problem what is wrong, naming the part, when anything is
         **/
        bool validateStrict(std::string * problem) const;

        v3d::type::geometry::AABBox bound(void) const;

        /**
         * Whether the mesh as a whole is selected, which is object mode selection.
         * Component selection is the flag each Vertex, HalfEdge and Face carries.
         **/
        bool selected(void) const noexcept;
        void selected(bool sel) noexcept;

        /**
         * Clear the selection flag of every vertex, edge and face, and leave the mesh's own
         * flag alone. A select mask change deselects components, not objects.
         **/
        void deselectComponents(void) noexcept;

        void addFace(const std::vector<glm::vec3> & vertices, const glm::vec3 & normal);

        size_t vertexCount(void) const;
        size_t edgeCount(void) const;
        size_t faceCount(void) const;

        Index addVertex(const Vertex & v);
        Index addEdge(const HalfEdge & newEdge);
        Index addFace(const Face & newFace);

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
