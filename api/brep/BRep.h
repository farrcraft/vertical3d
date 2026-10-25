/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/type/geometry/AABBox.h>

#include <cstdint>
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
 * It is geometry alone: it has no id and no placement, and is described about its own origin.
 * Whatever holds it gives it both. Each vertex, half edge and face carries its own selection
 * flag. A copy is a separate mesh with the same parts and the same flags.
 *
 * It counts its changes. Adding a part and restoring a snapshot each add one to changes(), and
 * so does every function in Operations.h, which work through them. A part changed in place,
 * through the pointer vertex(), edge() or face() returns, is not counted, so the code changing
 * it calls touch(). A change of selection is not a change of the geometry, and is not counted.
 **/
class BRep {
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
         * checked against. It checks everything validate() checks. It also checks that every
         * pair is mutual and every face's next chain closes. Every half edge in a face's loop
         * must name that face, and no two half edges of one face may run between the same two
         * vertices. An edge with no pair is allowed, because an open mesh has a boundary.
         *
         * @param problem what is wrong, naming the part, when anything is
         **/
        bool validateStrict(std::string * problem) const;

        v3d::type::geometry::AABBox bound(void) const;

        /**
         * The mesh's three arrays as they stand. A topology command keeps one from before an
         * operation and one from after it, and puts either back whole, so an undo restores the
         * numbering as well as the shape.
         **/
        struct Snapshot {
            std::vector<Vertex> vertices;
            std::vector<HalfEdge> edges;
            std::vector<Face> faces;
        };

        Snapshot snapshot(void) const;

        /**
         * Replace all three arrays with a snapshot's. The selection flags come back as the
         * snapshot holds them, so a caller that does not want them clears them after.
         **/
        void restore(const Snapshot & snapshot);

        /**
         * Clear the selection flag of every vertex, edge and face.
         **/
        void deselectComponents(void) noexcept;

        void addFace(const std::vector<glm::vec3> & vertices, const glm::vec3 & normal);

        /**
         * @return how many times the mesh has been changed. It never goes down
         **/
        std::uint64_t changes(void) const noexcept;

        /**
         * Count a change made to a part in place.
         **/
        void touch(void) noexcept;

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
        std::uint64_t changes_ = 0;
};


};  // namespace v3d::brep
