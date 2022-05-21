#ifndef INCLUDED_HOOKAH_FLTKKEYBOARD
#define INCLUDED_HOOKAH_FLTKKEYBOARD

#include "../../../input/KeyboardDevice.h"

#include <string>

namespace Hookah
{

	class FLTKKeyboard : public v3D::KeyboardDevice
	{
		public:
			void press(unsigned char keysym);
			void release(unsigned char keysym);

			bool tick();
	};

}; // end namespace Hookah

#endif // INCLUDED_HOOKAH_FLTKKEYBOARD
