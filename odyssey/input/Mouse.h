/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Device.h"

namespace odyssey::input {
	/**
	 **/
	class Mouse : public Device {
	public:
		/**
		 * Inherit base constructor
		 **/
		using Device::Device;

		/**
		 * @return bool true if the event was handled
		 **/
		bool handleEvent(const SDL_Event& event);
	};
};
