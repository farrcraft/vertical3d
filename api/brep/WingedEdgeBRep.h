/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/dag/Node.h>
#include <api/dag/Transform.h>
#include <api/type/geometry/AABBox.h>

#include <vector>

#include "Vertex.h"
#include "Edge.h"
#include "Face.h"

namespace v3d::brep {

/*
    http://www.baumgart.org/winged-edge/winged-edge.html
    http://www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/model/winged-e.html

    winged edge data structure
    each edge stores:
        - two vertices
        - (two) adjacent (left & right) faces
        - neighboring edges (wings)

    each face stores:
        - the first incident edge

    edge vertex stores:
        - the first incident edge

    4 wings:
    top left		next ccw
    top right		prev cw
    bottom left		next cw
    bottom right	prev ccw

    left face		next face
    right face		prev face

    top vertex		previous vertex
    bottom vertex	next vertex
*/

class WingedEdgeBRep : public v3d::dag::Node, public v3d::dag::Transform {
 public:
    WingedEdgeBRep();
    ~WingedEdgeBRep();

    static const Index INVALID_ID;

    class edge_iterator {
     public:
        edge_iterator();
        edge_iterator(WingedEdgeBRep * brep, Index faceID);
        ~edge_iterator();

        Edge * operator * ();
        edge_iterator operator++ (int);

        void reset(WingedEdgeBRep * brep, Index faceID);
        WingedEdgeBRep * brep(void) const;

     private:
        Edge * edge_;
        Index firstEdge_;
        bool winding_;
        WingedEdgeBRep * brep_;
    };


    class vertex_iterator {
     public:
        vertex_iterator();
        vertex_iterator(WingedEdgeBRep * brep, Index faceID);
        ~vertex_iterator();

        Vertex * operator * ();
        vertex_iterator operator++ (int);

        void reset(WingedEdgeBRep * brep, Index faceID);

     private:
        edge_iterator iterator_;
        bool nextVertex_;
    };


    Edge * edge(Index edgeID);
    Face * face(Index faceID);
    Vertex * vertex(Index vertexID);

    v3d::type::geometry::AABBox bound(void) const;

    bool selected(void) const noexcept;
    void selected(bool sel) noexcept;

    /*
        winding == true - ccw - face on left side of first edge
                    false - cw  - face on right side of first edge
    */
    void addFace(const std::vector<glm::vec3> & vertices, const glm::vec3 & normal, bool winding);
    void addEdge(const glm::vec3 & leftPoint, const glm::vec3 & rightPoint);

    void splitEdge(Index edgeID, const glm::vec3 & point);
    void extrudeFace(Index faceID);
    void splitFace(Index faceID, Index leftEdgeID, Index rightEdgeID, const glm::vec3 & leftPoint, const glm::vec3 & rightPoint);

    glm::vec3 center(Index faceID);
    void faceUV(Index faceID, glm::vec3 * u, glm::vec3 * v);

    size_t vertexCount(void) const;
    size_t edgeCount(void) const;
    size_t faceCount(void) const;

    // derived from dag::Transform
    virtual void translation(const glm::vec3 & t);
    virtual glm::vec3 translation(void) const;

    Index addVertex(const Vertex & v);
    Index addEdge(const Edge & e);
    Index addFace(const Face & f);

 protected:
    Index addVertex(const glm::vec3 & v);
    Index addEdge(Index leftVertex, Index rightVertex);

 private:
    std::vector<Vertex> vertices_;
    std::vector<Face> faces_;
    std::vector<Edge> edges_;
    bool selected_;
};

};  // namespace v3d::brep
