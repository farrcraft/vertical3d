/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */
#ifndef INCLUDED_V3D_INPUTDEVICE
#define INCLUDED_V3D_INPUTDEVICE

#include "../event/EventEmitter.h"

namespace v3D
{


	class InputDevice : public EventEmitter
	{
		public:
			virtual ~InputDevice() { }

			virtual bool tick() = 0;
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_INPUTDEVICE
