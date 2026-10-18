/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Scene.h"

#include <voxel/src/engine/Camera.h>
#include <voxel/src/engine/MortonCode.h>
#include <voxel/src/game/GameState.h>
#include <voxel/src/game/Player.h>
#include <voxel/src/voxel/Chunk.h>
#include <voxel/src/voxel/TerrainMap.h>

#include <cstddef>

#define GLM_ENABLE_EXPERIMENTAL 1

#include <glm/gtx/string_cast.hpp>

namespace {

/**
 * Where the player starts, above the terrain. The world is 64 blocks tall. There is no
 * collision or gravity, so a spawn under the hills would stay inside them.
 **/
constexpr glm::vec3 SPAWN_POSITION(128.0F, 80.0F, 240.0F);

};  // namespace

Scene::Scene() {
    player_.reset(new Player(SPAWN_POSITION));

    // generate the terrain heightmap
    TerrainMap terrain;
    terrain.generate();

    // block dimensions of the world / chunk size
    const size_t chunkSize = 16;
    const unsigned int worldHeight = 64 / chunkSize;
    const unsigned int worldWidth = 256 / chunkSize;
    const unsigned int worldDepth = 256 / chunkSize;

    unsigned int hash = 0;

    // populate world chunks
    boost::shared_ptr<Chunk> chunk;
    for (unsigned int x = 0; x < worldWidth; x++) {
        for (unsigned int y = 0; y < worldHeight; y++) {
            for (unsigned int z = 0; z < worldDepth; z++) {
                const glm::ivec3 pos(x, y, z);
                // Chunk scales the heightmap against a ceiling measured in blocks, so it
                // takes the world's block height, not its chunk count.
                chunk.reset(new Chunk(&terrain, pos, static_cast<unsigned int>(worldHeight * chunkSize)));
                hash = MortonCode::encode(pos);
                chunks_[hash] = chunk;
            }
        }
    }

    state_.reset(new GameState());
}

boost::unordered_map<unsigned int, boost::shared_ptr<Chunk> > & Scene::chunks() {
    return chunks_;
}


boost::shared_ptr<Player> Scene::player() {
    return player_;
}

boost::shared_ptr<GameState> Scene::state() {
    return state_;
}

boost::shared_ptr<Camera> Scene::camera() {
    return player_->camera();
}

void Scene::tick(float step) {
    player_->tick(step);
}

