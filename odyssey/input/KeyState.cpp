/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "KeyState.h"

#include <algorithm>

using namespace odyssey::input;

/**
 **/
bool KeyState::pressed(std::string_view c) const {
	std::vector<std::string_view>::const_iterator iter = std::find(keys_.begin(), keys_.end(), c);
	if (iter != keys_.end()) {
		return true;
	}
	return false;
}

/**
 **/
bool KeyState::operator() (std::string_view c) {
	std::vector<std::string_view>::const_iterator iter = std::find(keys_.begin(), keys_.end(), c);
	bool pressed = true;
	if (iter != keys_.end()) {
		keys_.erase(iter);
		pressed = false;
	}
	else {
		keys_.push_back(c);
	}
	return pressed;
}