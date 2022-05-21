/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../event/EventListener.h"

namespace v3d::input {

	/**
	 * An event listener for mouse events.
 	 */
	class MouseEventListener : public v3d::event::EventListener {
		public:
			virtual ~MouseEventListener() = default;

			virtual void motion(unsigned int x, unsigned int y) = 0;
			virtual void buttonPressed(unsigned int button) = 0;
			virtual void buttonReleased(unsigned int button) = 0;
	};


};
