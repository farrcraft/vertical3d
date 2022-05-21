#ifndef INCLUDED_HOOKAH_SFMLMOUSE
#define INCLUDED_HOOKAH_SFMLMOUSE

#include "../../../input/MouseDevice.h"

namespace Hookah
{

	class SFMLMouse : public v3D::MouseDevice
	{
		public:
			SFMLMouse();

			void press(int button);
			void release(int button);
			void motion(int x, int y);

			bool tick();

		private:
		  int buttons_;
	};

}; // end namespace Hookah

#endif // INCLUDED_HOOKAH_SFMLMOUSE
