#ifndef INCLUDED_HOOKAH_SDL2MOUSE
#define INCLUDED_HOOKAH_SDL2MOUSE

#include "../../../input/MouseDevice.h"

namespace Hookah
{

	class SDL2Mouse : public v3D::MouseDevice
	{
		public:
			bool tick();
	};

}; // end namespace Hookah

#endif // INCLUDED_HOOKAH_SDL2MOUSE
