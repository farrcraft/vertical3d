/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "InputDevice.h"

#include <string>
#include <vector>
#include <map>

namespace v3d::input {

	class KeyboardEventListener;

	class KeyboardDevice : public InputDevice {
		public:
			class KeyState {
				public:
					bool pressed(const std::string & c) const;
					bool operator() (const std::string & c);

				private:
					std::vector<std::string> keys_;
			};

			void addEventListener(KeyboardEventListener * listener, const std::string & name);
			bool removeEventListener(const std::string & name);

			/**
			* Event dispatch method for key press events notifies all bound listeners
			* @param key the key that has been pressed.
			*/
			void keyPressed(const std::string & key);
			/**
			* Event dispatch method for key release events.
			* @param key the key that has been released.
			*/
			void keyReleased(const std::string & key);

		protected:
			KeyState state_;

		private:
			std::map<std::string, KeyboardEventListener *> listeners_;
	};


};
