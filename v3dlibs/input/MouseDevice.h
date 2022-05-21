/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_V3D_MOUSEDEVICE
#define INCLUDED_V3D_MOUSEDEVICE

#include "InputDevice.h"
#include <map>
#include <vector>
#include <string>

namespace v3D
{

	class MouseEventListener;

	class MouseDevice : public InputDevice
	{
		public:
			class MouseState
			{
				public:
					typedef struct 
					{ 
						int position_[2];
					} MotionInfo;

					bool pressed(unsigned int button) const;
					MotionInfo position() const;

					/**
					 * Set the mouse position
					 * @param x the mouse x position
					 * @param y the mouse y position
					 * @return previous position
					 */
					MotionInfo operator() (unsigned int x, unsigned int y);
					/**
					 * Toggle the mouse button state
					 * @param button the button state to toggle
					 * @return whether the new button state is pressed or not
					 */
					bool operator() (unsigned int button);

					friend class MouseDevice;

				private:
					std::vector<unsigned int> buttons_;
					MotionInfo position_;
			};

			MouseDevice();
			void addEventListener(MouseEventListener * listener, const std::string & name);
			bool removeEventListener(const std::string & name);

			void motion(unsigned int x, unsigned int y);
			void buttonPressed(unsigned int button);
			void buttonReleased(unsigned int button);

		protected:
			MouseState state_;

		private:
			std::map<std::string, MouseEventListener *> listeners_;
	};


}; // end v3D namespace

#endif // INCLUDED_V3D_MOUSEDEVICE
