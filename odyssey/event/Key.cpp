/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Key.h"

using namespace odyssey::event;

/**
 **/
Key::Key(const std::string_view& name, bool pressed) :
	name_(name),
	pressed_(pressed) {
}

/**
**/
bool Key::pressed() const {
	return pressed_;
}

/**
**/
std::string_view Key::name() const {
	return name_;
}
