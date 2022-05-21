#include "../../Hookah.h"
#include "FLTKWindow.h"
#include "FLTKKeyboard.h"
#include "FLTKMouse.h"

#include <boost/shared_ptr.hpp>

boost::shared_ptr<Hookah::Window> Hookah::Create3DWindow(unsigned int width, unsigned int height)
{
	boost::shared_ptr<Hookah::Window> win;
	try
	{
		win.reset(new Hookah::FLTKWindow(width, height));
	}
	catch(std::exception & e)
	{
		throw e;
	}

	return win;
}

boost::shared_ptr<v3D::InputDevice> Hookah::CreateInputDevice(const std::string & device)
{
	boost::shared_ptr<v3D::InputDevice> dev;

	if (device == "keyboard")
	{
		dev.reset(new Hookah::FLTKKeyboard);
	}
	else if (device == "mouse")
	{
		dev.reset(new Hookah::FLTKMouse);
	}

	return dev;
}
