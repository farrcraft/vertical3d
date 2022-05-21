/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../event/EventEmitter.h"

namespace v3d::input {
	/**
	 **/
	class InputDevice : public v3d::event::EventEmitter {
		public:
			virtual ~InputDevice() = default;
			/**
			 **/
			virtual bool tick() = 0;
	};
};
