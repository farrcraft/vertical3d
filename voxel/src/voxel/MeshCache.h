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
    /** How many vertices, triangles and faces the cache starts with room for. **/
    struct Capacity {
        size_t vertices = 0;
        size_t tris = 0;
        size_t faces = 0;
    };

    explicit MeshCache(const Capacity& capacity);
    ~MeshCache();

    void reset();

    size_t addTri(size_t i0, size_t i1, size_t i2);
    size_t addTri(const glm::vec3 & v0, const glm::vec3 & v1, const glm::vec3 & v2);
    size_t addVertex(const glm::vec3 & vertex);

    /**
     * Cut the visible faces of a block into the cache.
     * @param voxel the block, whose position is in world blocks
     * @param faces which of its faces are not hidden
     * @param origin the corner of the chunk it belongs to, subtracted so that the geometry
     *        is chunk local and the chunk's position stays a push constant
     */
    void extract(const boost::shared_ptr<Voxel> & voxel, unsigned int faces, const glm::vec3 & origin);
    /**
     * A block type, the faces of the block that are drawn, and the one face this is. A face
     * not among those drawn is skipped.
     */
    struct Facing {
        unsigned int type = 0;
        unsigned int drawFaces = 0;
        unsigned int inFaces = 0;
    };

    void createFace(const Facing & facing, const glm::vec3 & v0, const glm::vec3 & v1, const glm::vec3 & v2, const glm::vec3 & v3);

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
