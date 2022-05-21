/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */
#ifndef INCLUDED_V3D_KEYBOARDEVENTLISTENER
#define INCLUDED_V3D_KEYBOARDEVENTLISTENER

#include "../event/EventListener.h"

namespace v3D
{

	class KeyboardEventListener : public EventListener
	{
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



}; // end namespace v3D

#endif // INCLUDED_V3D_KEYBOARDEVENTLISTENER
