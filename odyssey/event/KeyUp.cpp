/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "KeyUp.h"

using namespace odyssey::event;

/**
 **/
KeyUp::KeyUp(const std::string_view& name) :
	Key(name, false) {
}
