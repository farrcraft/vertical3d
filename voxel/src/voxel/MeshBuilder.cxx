/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "MeshBuilder.h"

#include <voxel/src/engine/MortonCode.h>

#include "Chunk.h"
#include "FaceCulling.h"
#include "MeshCache.h"
#include "ChunkMeshPool.h"

MeshBuilder::MeshBuilder(const boost::unordered_map<unsigned int, boost::shared_ptr<Chunk > > & chunks, const ChunkMeshBuilder & meshes) :
    chunks_(chunks),
    meshes_(meshes) {
    const size_t vertices = 32768;
    const size_t tris = 49152;
    const size_t faces = 24576;
    cache_.reset(new MeshCache(vertices, tris, faces));
}

void MeshBuilder::build(const boost::shared_ptr<ChunkMeshPool> & pool, size_t limit) {
    size_t updated = 0;
    // build mesh data from chunks
    for (boost::unordered_map<unsigned int, boost::shared_ptr<Chunk > >::iterator it = chunks_.begin(); it != chunks_.end(); it++) {
        // only generate mesh data for chunks that have active blocks in them
        if (!(*it).second->empty() && (*it).second->dirty()) {
            generateChunk(pool, (*it).second);
            updated++;
            if (updated == limit) {
                return;
            }
        }
    }
}


// generate a single mesh for the entire chunk
void MeshBuilder::generateChunk(const boost::shared_ptr<ChunkMeshPool> & pool, const boost::shared_ptr<Chunk> & chunk) {
    unsigned int hash = 0;
    glm::ivec3 pos;
    unsigned int faces = 0;

    glm::ivec3 chunkPosition = chunk->position();
    unsigned int chunkSize = chunk->size();

    size_t chunkId = MortonCode::encode(chunkPosition);
    // the mesh is built around the chunk's own corner, so the world position it is drawn at
    // is a push constant rather than something baked into every vertex
    glm::vec3 origin(chunkPosition * static_cast<int>(chunkSize));
    cache_->reset();  // reset cache not the ptr

    Voxel::BlockFace checkFaces[6] = {
        Voxel::BLOCK_FACE_FRONT, Voxel::BLOCK_FACE_BACK,
        Voxel::BLOCK_FACE_LEFT, Voxel::BLOCK_FACE_RIGHT,
        Voxel::BLOCK_FACE_TOP, Voxel::BLOCK_FACE_BOTTOM
    };
    boost::unordered_map<unsigned int, boost::shared_ptr<Voxel> > blocks = chunk->blocks();

    for (boost::unordered_map<unsigned int, boost::shared_ptr<Voxel> >::iterator it = blocks.begin(); it != blocks.end(); ++it) {
        // start with all possible faces
        faces = Voxel::BLOCK_FACE_FRONT|Voxel::BLOCK_FACE_BACK|Voxel::BLOCK_FACE_LEFT|Voxel::BLOCK_FACE_RIGHT|Voxel::BLOCK_FACE_TOP|Voxel::BLOCK_FACE_BOTTOM;

        hash = (*it).first;
        pos = MortonCode::decode3(hash);

        for (unsigned int i = 0; i < 6; i++) {
            // check for face occlusion from other blocks within the same chunk
            if (chunk->hidden(checkFaces[i], pos)) {
                faces &= ~checkFaces[i];
            } else {  // check for face occlusion from blocks in adjacent chunks
                Neighbour neighbor = neighbourAcrossSeam(checkFaces[i], pos, chunkPosition, static_cast<int>(chunkSize));
                if (neighbor.crosses) {
                    unsigned int neighborChunkHash = MortonCode::encode(neighbor.chunk);
                    boost::unordered_map<unsigned int, boost::shared_ptr<Chunk > >::iterator neighborChunk = chunks_.find(neighborChunkHash);
                    if (neighborChunk != chunks_.end()) {
                        if ((*neighborChunk).second->active(neighbor.block)) {
                            faces &= ~checkFaces[i];
                        }
                    }
                }
            }
        }
        cache_->extract((*it).second, faces, origin);
    }
    pool->add(chunkId, meshes_.build(cache_), origin);
    chunk->dirty(false);
}
