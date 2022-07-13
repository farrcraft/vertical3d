/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "KeyDown.h"

using namespace odyssey::event;

/**
 **/
KeyDown::KeyDown(const std::string_view& name) :
	Key(name, true) {
}
