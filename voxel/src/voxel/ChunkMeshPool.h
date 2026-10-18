/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/memory/Mesh.h>

#include <functional>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <glm/vec3.hpp>

/**
 * The meshed chunks, held by the app.
 *
 * A chunk that is remeshed replaces its entry. A frame in flight may still be drawing the mesh
 * that was there, so the pool hands it to a retire function rather than destroying it. The
 * renderer's retire function holds it until every frame that may name it has finished.
 **/
class ChunkMeshPool {
 public:
    /**
     * Takes what destroys a replaced mesh, and holds it until that is safe.
     **/
    using Retire = std::function<void(std::function<void()>)>;

    /**
     * One chunk's geometry and where in the world it sits.
     **/
    struct Entry {
        Entry();

        boost::shared_ptr<v3d::render::realtime::vulkan::memory::Mesh> mesh;
        glm::vec3 origin;  /**< the chunk's corner in blocks, which its vertices are relative to **/
        float size;        /**< how many blocks the chunk spans along each axis **/
    };

    typedef boost::unordered_map<size_t, Entry> EntryMap;

    /**
     * @param retire what a replaced mesh is handed to
     **/
    explicit ChunkMeshPool(Retire retire);

    /**
     * Take a chunk's mesh, replacing whatever was there and retiring it.
     **/
    void add(size_t chunkId, const boost::shared_ptr<v3d::render::realtime::vulkan::memory::Mesh>& mesh, const glm::vec3& origin,
        float size);

    /**
     * @return the meshes, for a caller building a draw item out of each
     **/
    const EntryMap& entries() const;

 private:
    Retire retire_;
    EntryMap pool_;
};
