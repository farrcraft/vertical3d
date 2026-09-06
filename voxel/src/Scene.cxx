/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Scene.h"

#include "game/GameState.h"
#include "game/Player.h"

#include "engine/Camera.h"
#include "engine/MortonCode.h"

#include "voxel/Chunk.h"
#include "voxel/TerrainMap.h"

#define GLM_ENABLE_EXPERIMENTAL 1

#include <glm/gtx/string_cast.hpp>

Scene::Scene() {
    // above the terrain rather than inside it - the world is 64 blocks tall, and there is no
    // collision or gravity to lift a spawn that starts under the hills back out of them
    player_.reset(new Player(glm::vec3(128.0f, 80.0f, 240.0f)));

    // generate the terrain heightmap
    TerrainMap terrain;
    terrain.generate();

    // block dimensions of the world / chunk size
    const size_t chunkSize = 16;
    unsigned int worldHeight = 64 / chunkSize;
    unsigned int worldWidth = 256 / chunkSize;
    unsigned int worldDepth = 256 / chunkSize;

    MortonCode encoder;
    unsigned int hash = 0;

    // populate world chunks
    boost::shared_ptr<Chunk> chunk;
    for (unsigned int x = 0; x < worldWidth; x++) {
        for (unsigned int y = 0; y < worldHeight; y++) {
            for (unsigned int z = 0; z < worldDepth; z++) {
                glm::ivec3 pos(x, y, z);
                // Chunk scales the heightmap against a ceiling measured in blocks, so it
                // wants the world's block height, not its chunk count.
                chunk.reset(new Chunk(&terrain, pos, static_cast<unsigned int>(worldHeight * chunkSize)));
                hash = encoder.encode(pos);
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

