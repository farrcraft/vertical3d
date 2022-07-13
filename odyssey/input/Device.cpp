/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Device.h"

using namespace odyssey::input;

Device::Device(const boost::shared_ptr<entt::dispatcher>& dispatcher) : 
	dispatcher_(dispatcher) {

}
