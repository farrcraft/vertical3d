/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "VertexBufferBuilder.h"

#include <GL/glew.h>

#include <vector>

#include "../voxel/MeshCache.h"

#include <glm/gtc/type_ptr.hpp>
#include <boost/make_shared.hpp>

boost::shared_ptr<v3d::gl::VertexBuffer> VertexBufferBuilder::build(const boost::shared_ptr<MeshCache> & mesh) {
    boost::shared_ptr<v3d::gl::VertexBuffer> buffer;
    buffer = boost::make_shared<v3d::gl::VertexBuffer>(v3d::gl::VertexBuffer::BUFFER_TYPE_STATIC);

    glm::ivec4 * faces = mesh->faces();
    unsigned int faceCount = mesh->faceCount();
    // attribute 1 is per-vertex, and a face is a four-vertex quad.
    std::vector<glm::vec2> info;
    info.reserve(faceCount * 4);

    glm::vec3 * meshVertices = mesh->vertices();
    glm::ivec3 * meshTris = mesh->tris();
    unsigned int vertexCount = mesh->vertexCount();
    for (unsigned int i = 0; i < faceCount; i++) {
        glm::ivec4 face = faces[i];

        for (unsigned int corner = 0; corner < 4; corner++) {
            info.push_back(glm::vec2(face.z, face.w));
        }
    }
    buffer->attribute(0, 3, v3d::gl::VertexBuffer::ATTRIBUTE_TYPE_VERTEX, vertexCount);
    buffer->attribute(1, 2, v3d::gl::VertexBuffer::ATTRIBUTE_TYPE_GENERIC, info.size());

    buffer->allocate();
    buffer->set(0, glm::value_ptr(meshVertices[0]), vertexCount);
    buffer->data2f(1, info);
    buffer->indices(meshTris, mesh->triCount() * 3);
    return buffer;
}
