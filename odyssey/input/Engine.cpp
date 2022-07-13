/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Engine.h"
#include "Keyboard.h"
#include "Mouse.h"

#include <boost/make_shared.hpp>

using namespace odyssey::input;

/**
 **/
Engine::Engine(const boost::shared_ptr<entt::dispatcher>& dispatcher) : 
	dispatcher_(dispatcher) {

	// add keyboard & mouse devices
	devices_.push_back(boost::make_shared<Keyboard>(dispatcher_));
	devices_.push_back(boost::make_shared<Mouse>(dispatcher_));
}

/**
 **/
bool Engine::filterEvent(const SDL_Event& event) {
	// iterate over devices
	for (auto it = devices_.begin(); it != devices_.end(); it++) {
		if ((*it)->handleEvent(event)) {
			return true;
		}
	}
	return false;
}
