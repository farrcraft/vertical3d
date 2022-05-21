/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "../event/EventListener.h"

namespace v3d::input
{

	class KeyboardEventListener : public v3d::event::EventListener {
		public:
			virtual ~KeyboardEventListener() { }

			/**
			* Event notification method for key press events.
			* @param key the key that has been pressed.
			*/
			virtual void keyPressed(const std::string & key) = 0;
			/**
			* Event notification method for key release events.
			* @param key the key that has been released.
			*/
			virtual void keyReleased(const std::string & key) = 0;
	};



};