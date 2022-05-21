#ifndef INCLUDED_HOOKAH_SFMLKEYBOARD
#define INCLUDED_HOOKAH_SFMLKEYBOARD

#include "../../../input/KeyboardDevice.h"

#include <string>

namespace Hookah
{

	class SFMLKeyboard : public v3D::KeyboardDevice
	{
		public:
			void press(const std::string & evnt);
			void release(const std::string & evnt);

			bool tick();
	};

}; // end namespace Hookah

#endif // INCLUDED_HOOKAH_SFMLKEYBOARD
