/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/Mesh.h>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <glm/vec3.hpp>

/**
 * The meshed chunks, held by the app because nothing in the engine owns geometry - ADR-0010.
 *
 * A chunk that is remeshed replaces its entry, and the mesh that was there is destroyed with
 * the last reference to it. Which is why a remesh has to happen while nothing is in flight
 * against the old buffers - the pool is only written from a tick, before the frame that
 * reads it is recorded.
 **/
class ChunkMeshPool {
 public:
    /**
     * One chunk's geometry and where in the world it sits.
     **/
    struct Entry {
        Entry();

        boost::shared_ptr<v3d::render::realtime::vulkan::Mesh> mesh;
        glm::vec3 origin;  /**< the chunk's corner in blocks, which its vertices are relative to **/
    };

    typedef boost::unordered_map<size_t, Entry> EntryMap;

    /**
     * Take a chunk's mesh, replacing whatever was there.
     **/
    void add(size_t chunkId, const boost::shared_ptr<v3d::render::realtime::vulkan::Mesh>& mesh, const glm::vec3& origin);

    /**
     * @return the meshes, for a caller building a draw item out of each
     **/
    const EntryMap& entries() const;

 private:
    EntryMap pool_;
};
