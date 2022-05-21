/**
 * HooKaH - 3D Application Framework Library 
 * (c) Josh Farr <j.wgasa@gmail.com>
 * Version 0.0.8
 * 
 **/
#ifndef INCLUDED_HOOKAH_HOOKAH
#define INCLUDED_HOOKAH_HOOKAH

#include "Window.h"
#include "../input/InputDevice.h"

namespace Hookah
{

	// Global Engine Methods

	/**
	 * Create a new 3D window.
	 * This a stub function that is defined in a a backend driver that actually implements
	 * the window creation. (E.g., SDL & FLTK each handle window creation in their own way).
	 *
	 * @param width the width of the new window
	 * @param height the height of the new window
	 * @return a pointer to the new window
	 */
	boost::shared_ptr<Window> Create3DWindow(unsigned int width, unsigned int height);

	/**
	 * Create a new input device object for a given device name.
	 * Input device creation is dependent on the backend driver. The implementation specific
	 * details are hidden in the driver libraries.
	 * 
	 * @param device the name of the input device object to create. Valid options are driver
	 *				 specific, but common names include "mouse" and "keyboard".
	 * @return a pointer to the new input device object
	 */
	boost::shared_ptr<v3D::InputDevice> CreateInputDevice(const std::string & device);

}; // end namespace Hookah

#endif // INCLUDED_HOOKAH_HOOKAH
