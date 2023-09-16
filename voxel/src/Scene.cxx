/**
 * Voxel Engine
 *
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#include "Scene.h"

#include "game/GameState.h"
#include "game/Player.h"

#include "engine/Camera.h"
#include "engine/MortonCode.h"

#include "voxel/Chunk.h"
#include "voxel/TerrainMap.h"

#include <glm/gtx/string_cast.hpp>

Scene::Scene()
{
    player_.reset(new Player(glm::vec3(0.0f, 25.0f, 100.0f)));

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
    for (unsigned int x = 0; x < worldWidth; x++)
    {
        for (unsigned int y = 0; y < worldHeight; y++)
        {
            for (unsigned int z = 0; z < worldDepth; z++)
            {
                glm::ivec3 pos(x, y, z);
                chunk.reset(new Chunk(terrain, pos, worldHeight));
                hash = encoder.encode(pos);
                chunks_[hash] = chunk;
            }
        }
    }

    state_.reset(new GameState());
}

boost::unordered_map<unsigned int, boost::shared_ptr<Chunk> > & Scene::chunks()
{
    return chunks_;
}


boost::shared_ptr<Player> Scene::player()
{
    return player_;
}

boost::shared_ptr<GameState> Scene::state()
{
    return state_;
}

boost::shared_ptr<Camera> Scene::camera()
{
    return player_->camera();
}

void Scene::tick(unsigned int delta)
{
    player_->tick(delta);
}

