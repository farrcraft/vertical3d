/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <vector>

#include "Vertex.h"
#include "HalfEdge.h"
#include "Face.h"

#include "../type/AABBox.h"

#include <boost/shared_ptr.hpp>

namespace v3d::brep {

    class BRep {
     public:
            BRep();
            ~BRep();

            static const uint32_t INVALID_ID;

            class edge_iterator {
             public:
                    edge_iterator();
                    edge_iterator(boost::shared_ptr<BRep> brep, unsigned int face);
                    ~edge_iterator();

                    HalfEdge * operator * ();
                    edge_iterator operator++ (int);

                    void reset(boost::shared_ptr<BRep> brep, unsigned int face);
                    boost::shared_ptr<BRep> brep(void) const;

             private:
                    HalfEdge * edge_;
                    uint64_t firstEdge_;
                    boost::shared_ptr<BRep> brep_;
            };

            class vertex_iterator {
             public:
                    vertex_iterator();
                    vertex_iterator(boost::shared_ptr<BRep> brep, unsigned int face);
                    ~vertex_iterator();

                    Vertex * operator * ();
                    vertex_iterator operator++ (int);

                    void reset(boost::shared_ptr<BRep> brep, unsigned int face);

             private:
                    edge_iterator iterator_;
            };

            HalfEdge * edge(unsigned int e);
            Face * face(unsigned int f);
            Vertex * vertex(unsigned int vert);

            v3d::type::AABBox bound(void) const;

            void addFace(const std::vector<glm::vec3> & vertices, const glm::vec3 & normal);
            void addEdge(const glm::vec3 & point);

            void splitEdge(unsigned int edge, const glm::vec3 & point);
            void extrudeFace(unsigned int face);
            void splitFace(unsigned int face, unsigned int leftEdge, unsigned int rightEdge, const glm::vec3 & leftPoint, const glm::vec3 & rightPoint);

            size_t vertexCount(void) const;
            size_t edgeCount(void) const;
            size_t faceCount(void) const;

            size_t addVertex(const Vertex & v);
            size_t addEdge(const HalfEdge & e);
            size_t addFace(const Face & f);

     protected:
            size_t addVertex(const glm::vec3 & v);
            size_t addEdge(unsigned int vertex);
            size_t findPair(unsigned int edge, unsigned int prevEdge);

     private:
            std::vector<Vertex> vertices_;
            std::vector<Face> faces_;
            std::vector<HalfEdge> edges_;
    };

    /**
     * Get the mid point of the face of a mesh
     * @param mesh the brep
     * @param face the face number to get the center of
     * @return the point located in the middle of the face
     */
    glm::vec3 center(boost::shared_ptr<BRep> mesh, unsigned int face);
    /**
     * Get the UV vectors for a mesh face
     * @param mesh the mesh to use
     * @param face the face number to get the coordinates for
     * @param u the address of a vector to store the results in
     * @param v the address of a vector to store the results in
     */
    void faceUV(boost::shared_ptr<BRep> mesh, unsigned int face, glm::vec3 & u, glm::vec3 & v);

};  // namespace v3d::brep
