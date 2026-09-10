/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/dag/Node.h>
#include <api/dag/Transform.h>
#include <api/type/geometry/AABBox.h>

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

        class edge_iterator {
         public:
                edge_iterator();
                edge_iterator(boost::shared_ptr<BRep> brep, Index face);
                ~edge_iterator();

                HalfEdge * operator * ();
                edge_iterator operator++ (int);

                void reset(boost::shared_ptr<BRep> brep, Index face);
                boost::shared_ptr<BRep> brep(void) const;

         private:
                HalfEdge * edge_;
                Index firstEdge_;
                boost::shared_ptr<BRep> brep_;
        };

        class vertex_iterator {
         public:
                vertex_iterator();
                vertex_iterator(boost::shared_ptr<BRep> brep, Index face);
                ~vertex_iterator();

                Vertex * operator * ();
                vertex_iterator operator++ (int);

                void reset(boost::shared_ptr<BRep> brep, Index face);

         private:
                edge_iterator iterator_;
        };

        HalfEdge * edge(Index e);
        Face * face(Index f);
        Vertex * vertex(Index vert);

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

        void splitEdge(Index edge, const glm::vec3 & point);
        void extrudeFace(Index face);
        void splitFace(Index face, Index leftEdge, Index rightEdge, const glm::vec3 & leftPoint, const glm::vec3 & rightPoint);

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

/**
 * Get the mid point of the face of a mesh
 * @param mesh the brep
 * @param face the face number to get the center of
 * @return the point located in the middle of the face
 */
glm::vec3 center(boost::shared_ptr<BRep> mesh, Index face);
/**
 * Get the UV vectors for a mesh face
 * @param mesh the mesh to use
 * @param face the face number to get the coordinates for
 * @param u the address of a vector to store the results in
 * @param v the address of a vector to store the results in
 */
void faceUV(boost::shared_ptr<BRep> mesh, Index face, glm::vec3* u, glm::vec3* v);

};  // namespace v3d::brep
