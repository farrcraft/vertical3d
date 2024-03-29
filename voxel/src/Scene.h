/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#pragma once

#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

class Chunk;
class GameState;
class Player;
class Camera;

class Scene {
 public:
    Scene();

    void tick(unsigned int delta);

    boost::shared_ptr<Player> player();
    boost::shared_ptr<GameState> state();
    boost::shared_ptr<Camera> camera();

    boost::unordered_map<unsigned int, boost::shared_ptr<Chunk> > & chunks();

 private:
    boost::shared_ptr<Player> player_;
    boost::shared_ptr<GameState> state_;
    boost::unordered_map<unsigned int, boost::shared_ptr<Chunk> > chunks_;
};
