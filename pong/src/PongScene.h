/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Ball.h"
#include "Paddle.h"
#include "GameState.h"

#include "../../v3dlibs/audio/SoundEngine.h"

#include <boost/property_tree/ptree.hpp>

class PongScene {
 public:
    PongScene(const boost::property_tree::ptree & ptree, const std::string & assetPath);
    ~PongScene();

    void tick(unsigned int delta);
    void resize(int width, int height);

    void reset();

    Ball & ball();
    Paddle & left();
    Paddle & right();
    GameState & state();

 private:
    Ball ball_;
    Paddle left_, right_;
    GameState gameState_;
    int width_, height_;
    v3d::audio::SoundEngine soundEngine_;
};
