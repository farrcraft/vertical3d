/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "MeshCache.h"

#include <demos/voxel/src/voxel/Voxel.h>

#include <cstddef>
#include <stdexcept>

MeshCache::MeshCache(const Capacity& capacity) :
    vertices_(0),
    tris_(0),
    faces_(0),
    vertexCount_(0),
    triCount_(0),
    faceCount_(0),
    maxVertices_(capacity.vertices),
    maxTris_(capacity.tris),
    maxFaces_(capacity.faces) {
    if (capacity.vertices > 0) {
        vertices_ = new glm::vec3[capacity.vertices];
    }
    if (capacity.tris > 0) {
        tris_ = new glm::ivec3[capacity.tris];
    }
    if (capacity.faces > 0) {
        faces_ = new glm::ivec4[capacity.faces];
    }
}

MeshCache::~MeshCache() {
    delete [] vertices_;
    delete [] tris_;
    delete [] faces_;
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

size_t MeshCache::addTri(size_t i0, size_t i1, size_t i2) {
    if (triCount_ == maxTris_) {
        throw std::runtime_error("MeshCache tri limit exceeded!");
    }
    const size_t index = triCount_;
    triCount_++;
    tris_[index] = glm::ivec3(i0, i1, i2);
    return index;
}

size_t MeshCache::addTri(const glm::vec3 & v0, const glm::vec3 & v1, const glm::vec3 & v2) {
    const size_t p0 = addVertex(v0);
    const size_t p1 = addVertex(v1);
    const size_t p2 = addVertex(v2);

    const size_t t1 = addTri(p0, p1, p2);
    return t1;
}

size_t MeshCache::addVertex(const glm::vec3 & vertex) {
    if (vertexCount_ == maxVertices_) {
        throw std::runtime_error("MeshCache vertex limit exceeded!");
    }
    const size_t index = vertexCount_;
    vertexCount_++;
    vertices_[index] = vertex;
    return index;
}

void MeshCache::extract(const boost::shared_ptr<Voxel> & voxel, unsigned int faces, const glm::vec3 & origin) {
    if (!voxel->active() || (faces & Voxel::BLOCK_FACE_NONE) != 0) {
        return;
    }

    if ((faces & Voxel::BLOCK_FACE_ALL) != 0) {
        faces = Voxel::BLOCK_FACE_FRONT|Voxel::BLOCK_FACE_BACK|Voxel::BLOCK_FACE_LEFT|Voxel::BLOCK_FACE_RIGHT|Voxel::BLOCK_FACE_TOP|Voxel::BLOCK_FACE_BOTTOM;
    }

    const glm::vec3 position = voxel->position() - origin;
    const unsigned int type = voxel->type();

    // front
    const glm::vec3 p0 = glm::vec3(0.0F, 0.0F, 1.0F) + position;
    const glm::vec3 p1 = glm::vec3(1.0F, 0.0F, 1.0F) + position;
    const glm::vec3 p2 = glm::vec3(1.0F, 1.0F, 1.0F) + position;
    const glm::vec3 p3 = glm::vec3(0.0F, 1.0F, 1.0F) + position;
    // back
    const glm::vec3 p4 = glm::vec3(0.0F, 0.0F, 0.0F) + position;
    const glm::vec3 p5 = glm::vec3(1.0F, 0.0F, 0.0F) + position;
    const glm::vec3 p6 = glm::vec3(1.0F, 1.0F, 0.0F) + position;
    const glm::vec3 p7 = glm::vec3(0.0F, 1.0F, 0.0F) + position;

    createFace({.type = type, .drawFaces = faces, .inFaces = Voxel::BLOCK_FACE_FRONT},  p0, p1, p2, p3);
    createFace({.type = type, .drawFaces = faces, .inFaces = Voxel::BLOCK_FACE_RIGHT},  p1, p5, p6, p2);
    createFace({.type = type, .drawFaces = faces, .inFaces = Voxel::BLOCK_FACE_TOP},    p2, p6, p7, p3);
    createFace({.type = type, .drawFaces = faces, .inFaces = Voxel::BLOCK_FACE_LEFT},   p4, p0, p3, p7);
    createFace({.type = type, .drawFaces = faces, .inFaces = Voxel::BLOCK_FACE_BOTTOM}, p4, p5, p1, p0);
    createFace({.type = type, .drawFaces = faces, .inFaces = Voxel::BLOCK_FACE_BACK},   p5, p4, p7, p6);
}

void MeshCache::createFace(const Facing & facing,
    const glm::vec3 & v0, const glm::vec3 & v1, const glm::vec3 & v2, const glm::vec3 & v3) {
    const unsigned int type = facing.type;
    const unsigned int inFaces = facing.inFaces;
    if ((facing.drawFaces & facing.inFaces) == 0) {
        return;
    }

    // the two triangles share an edge, so the quad is four vertices indexed six times. Do not
    // use addTri(vec3, vec3, vec3) here. It appends three new vertices per triangle, which
    // leaves the index buffer as 0, 1, 2, 3 and so on, carrying no information.
    const size_t p0 = addVertex(v0);
    const size_t p1 = addVertex(v1);
    const size_t p2 = addVertex(v2);
    const size_t p3 = addVertex(v3);

    const size_t t1 = addTri(p0, p1, p2);
    const size_t t2 = addTri(p0, p2, p3);

    if (faceCount_ == maxFaces_) {
        throw std::runtime_error("MeshCache face limit exceeded!");
    }
    const size_t index = faceCount_;
    faceCount_++;
    faces_[index] = glm::ivec4(t1, t2, inFaces, type);
}
