/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#pragma once

#include <glm/glm.hpp>

#include <boost/shared_ptr.hpp>

class Voxel;

class MeshCache {
 public:
    MeshCache(size_t vertices, size_t tris, size_t faces);
    ~MeshCache();

    void reset();

    size_t addTri(size_t a, size_t b, size_t c);
    size_t addTri(const glm::vec3 & v0, const glm::vec3 & v1, const glm::vec3 & v2);
    size_t addVertex(const glm::vec3 & vertex);

    void extract(const boost::shared_ptr<Voxel> & voxel, unsigned int faces);
    void createFace(unsigned int type, unsigned int drawFaces, unsigned int inFaces, const glm::vec3 & v0, const glm::vec3 & v1, const glm::vec3 & v2, const glm::vec3 & v3);

    glm::vec3 * vertices();
    glm::ivec3 * tris();
    glm::ivec4 * faces();

    size_t triCount() const;
    size_t faceCount() const;
    size_t vertexCount() const;

 private:
    glm::vec3 * vertices_;
    glm::ivec3 * tris_;
    glm::ivec4 * faces_;
    size_t vertexCount_;
    size_t triCount_;
    size_t faceCount_;
    size_t maxVertices_;
    size_t maxTris_;
    size_t maxFaces_;
};
