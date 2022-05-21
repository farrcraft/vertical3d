#ifndef INCLUDED_HOOKAH_FLTKMOUSE
#define INCLUDED_HOOKAH_FLTKMOUSE

#include "../../../input/MouseDevice.h"

namespace Hookah
{

	class FLTKMouse : public v3D::MouseDevice
	{
		public:
			FLTKMouse();

			void press(int button);
			void release(int button);
			void motion(int x, int y);

			bool tick();

		private:
		  int buttons_;
	};

}; // end namespace Hookah

#endif // INCLUDED_HOOKAH_FLTKMOUSE
