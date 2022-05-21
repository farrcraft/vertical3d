#ifndef INCLUDED_HOOKAH_SDLMOUSE
#define INCLUDED_HOOKAH_SDLMOUSE

#include "../../../input/MouseDevice.h"

namespace Hookah
{

	class SDLMouse : public v3D::MouseDevice
	{
		public:
			bool tick();
	};

}; // end namespace Hookah

#endif // INCLUDED_HOOKAH_SDLMOUSE
