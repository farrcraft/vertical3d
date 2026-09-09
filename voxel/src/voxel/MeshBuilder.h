/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#pragma once

#include <voxel/src/engine/ChunkMeshBuilder.h>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

class Chunk;
class ChunkMeshPool;
class MeshCache;

class MeshBuilder {
 public:
    MeshBuilder(const boost::unordered_map<unsigned int, boost::shared_ptr<Chunk > > & chunks, const ChunkMeshBuilder & meshes);

    void build(const boost::shared_ptr<ChunkMeshPool> & pool, size_t limit);

 protected:
    /**
    * Genereate mesh geometry for a chunk
    */
    void generateChunk(const boost::shared_ptr<ChunkMeshPool> & pool, const boost::shared_ptr<Chunk> & chunk);

 private:
    boost::shared_ptr<MeshCache> cache_;
    boost::unordered_map<unsigned int, boost::shared_ptr<Chunk > > chunks_;
    ChunkMeshBuilder meshes_;
};
