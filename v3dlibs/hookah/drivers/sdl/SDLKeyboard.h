#ifndef INCLUDED_HOOKAH_SDLKEYBOARD
#define INCLUDED_HOOKAH_SDLKEYBOARD

#include "../../../input/KeyboardDevice.h"

#include <string>

namespace Hookah
{

	class SDLKeyboard : public v3D::KeyboardDevice
	{
		public:
			bool tick();
	};

}; // end namespace Hookah

#endif // INCLUDED_HOOKAH_SDLKEYBOARD
