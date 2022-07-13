/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Scene.h"

#include <boost/make_shared.hpp>

using namespace odyssey::render;

/**
 **/
Scene::Scene(boost::shared_ptr<Context> context) :
	context_(context) {

}

/**
 **/
boost::shared_ptr<Frame> Scene::collect() {
	boost::shared_ptr<Frame> frame = boost::make_shared<Frame>(context_);

	// do stuff with frame
	player_->draw(frame);

	return frame;
}

/**
 **/
void Scene::setPlayer(boost::shared_ptr<renderable::Player> player) {
	player_ = player;
}
