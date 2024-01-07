/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Scene.h"

#include <boost/make_shared.hpp>

namespace odyssey::render {

/**
 **/
boost::shared_ptr<v3d::render::realtime::Frame> Scene::collect() {
    boost::shared_ptr<v3d::render::realtime::Frame> frame = boost::make_shared<v3d::render::realtime::Frame>(context_);

    // do stuff with frame
    player_->draw(frame);

    return frame;
}

/**
 **/
void Scene::setPlayer(boost::shared_ptr<renderable::Player> player) {
    player_ = player;
}

};  // namespace odyssey::render
