/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#pragma once

#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <glm/vec3.hpp>

class Chunk;
class GameState;
class Player;

class Scene {
 public:
    Scene();

    /**
     * Advance the world by one simulation step.
     * @param step seconds of simulated time
     */
    void tick(float step);

    boost::shared_ptr<Player> player();
    boost::shared_ptr<GameState> state();

    boost::unordered_map<unsigned int, boost::shared_ptr<Chunk> > & chunks();

 private:
    /**
     * Where the player starts, above the terrain. The world is 64 blocks tall. There is no
     * collision or gravity, so a spawn under the hills would stay inside them.
     **/
    static constexpr glm::vec3 SPAWN_POSITION{128.0F, 80.0F, 240.0F};

    boost::shared_ptr<Player> player_;
    boost::shared_ptr<GameState> state_;
    boost::unordered_map<unsigned int, boost::shared_ptr<Chunk> > chunks_;
};
