/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "MeshCache.h"

#include <stdexcept>

#include "../voxel/Voxel.h"

MeshCache::MeshCache(size_t vertices, size_t tris, size_t faces) :
    triCount_(0),
    vertexCount_(0),
    faceCount_(0),
    maxVertices_(vertices),
    maxFaces_(faces),
    maxTris_(tris),
    vertices_(0),
    tris_(0),
    faces_(0) {
    if (vertices > 0) {
        vertices_ = new glm::vec3[vertices];
    }
    if (tris > 0) {
        tris_ = new glm::ivec3[tris];
    }
    if (faces > 0) {
        faces_ = new glm::ivec4[faces];
    }
}

MeshCache::~MeshCache() {
    if (vertices_) {
        delete [] vertices_;
    }
    if (tris_) {
        delete [] tris_;
    }
    if (faces_) {
        delete [] faces_;
    }
}

glm::vec3 * MeshCache::vertices() {
    return vertices_;
}

glm::ivec3 * MeshCache::tris() {
    return tris_;
}

glm::ivec4 * MeshCache::faces() {
    return faces_;
}

size_t MeshCache::triCount() const {
    return triCount_;
}

size_t MeshCache::faceCount() const {
    return faceCount_;
}

size_t MeshCache::vertexCount() const {
    return vertexCount_;
}

void MeshCache::reset() {
    triCount_ = 0;
    faceCount_ = 0;
    vertexCount_ = 0;
}

size_t MeshCache::addTri(size_t a, size_t b, size_t c) {
    if (triCount_ == maxTris_) {
        throw std::runtime_error("MeshCache tri limit exceeded!");
    }
    unsigned int index = triCount_;
    triCount_++;
    tris_[index] = glm::ivec3(a, b, c);
    return index;
}

size_t MeshCache::addTri(const glm::vec3 & v0, const glm::vec3 & v1, const glm::vec3 & v2) {
    size_t p0 = addVertex(v0);
    size_t p1 = addVertex(v1);
    size_t p2 = addVertex(v2);

    size_t t1 = addTri(p0, p1, p2);
    return t1;
}

size_t MeshCache::addVertex(const glm::vec3 & vertex) {
    if (vertexCount_ == maxVertices_) {
        throw std::runtime_error("MeshCache vertex limit exceeded!");
    }
    unsigned int index = vertexCount_;
    vertexCount_++;
    vertices_[index] = vertex;
    return index;
}

void MeshCache::extract(const boost::shared_ptr<Voxel> & voxel, unsigned int faces) {
    if (!voxel->active() || (faces & Voxel::BLOCK_FACE_NONE)) {
        return;
    }

    if (faces & Voxel::BLOCK_FACE_ALL) {
        faces = Voxel::BLOCK_FACE_FRONT|Voxel::BLOCK_FACE_BACK|Voxel::BLOCK_FACE_LEFT|Voxel::BLOCK_FACE_RIGHT|Voxel::BLOCK_FACE_TOP|Voxel::BLOCK_FACE_BOTTOM;
    }

    glm::vec3 position = voxel->position();
    unsigned int type = voxel->type();

    // front
    glm::vec3 p0 = glm::vec3(0.0f, 0.0f, 1.0f) + position;
    glm::vec3 p1 = glm::vec3(1.0f, 0.0f, 1.0f) + position;
    glm::vec3 p2 = glm::vec3(1.0f, 1.0f, 1.0f) + position;
    glm::vec3 p3 = glm::vec3(0.0f, 1.0f, 1.0f) + position;
    // back
    glm::vec3 p4 = glm::vec3(0.0f, 0.0f, 0.0f) + position;
    glm::vec3 p5 = glm::vec3(1.0f, 0.0f, 0.0f) + position;
    glm::vec3 p6 = glm::vec3(1.0f, 1.0f, 0.0f) + position;
    glm::vec3 p7 = glm::vec3(0.0f, 1.0f, 0.0f) + position;

    createFace(type, faces, Voxel::BLOCK_FACE_FRONT,  p0, p1, p2, p3);
    createFace(type, faces, Voxel::BLOCK_FACE_RIGHT,  p1, p5, p6, p2);
    createFace(type, faces, Voxel::BLOCK_FACE_TOP,    p2, p6, p7, p3);
    createFace(type, faces, Voxel::BLOCK_FACE_LEFT,   p4, p0, p3, p7);
    createFace(type, faces, Voxel::BLOCK_FACE_BOTTOM, p4, p5, p1, p0);
    createFace(type, faces, Voxel::BLOCK_FACE_BACK,   p5, p4, p7, p6);
}

void MeshCache::createFace(unsigned int type, unsigned int drawFaces, unsigned int inFaces,
    const glm::vec3 & v0, const glm::vec3 & v1, const glm::vec3 & v2, const glm::vec3 & v3) {
    if (!(drawFaces & inFaces)) {
        return;
    }

    size_t t1 = addTri(v0, v1, v2);
    size_t t2 = addTri(v0, v2, v3);

    if (faceCount_ == maxFaces_) {
        throw std::runtime_error("MeshCache face limit exceeded!");
    }
    size_t index = faceCount_;
    faceCount_++;
    faces_[index] = glm::ivec4(t1, t2, inFaces, type);
}
