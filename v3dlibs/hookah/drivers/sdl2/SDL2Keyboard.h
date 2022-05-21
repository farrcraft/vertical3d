#ifndef INCLUDED_HOOKAH_SDL2KEYBOARD
#define INCLUDED_HOOKAH_SDL2KEYBOARD

#include "../../../input/KeyboardDevice.h"

#include <string>

namespace Hookah
{

	class SDL2Keyboard : public v3D::KeyboardDevice
	{
		public:
			bool tick();
	};

}; // end namespace Hookah

#endif // INCLUDED_HOOKAH_SDL2KEYBOARD
