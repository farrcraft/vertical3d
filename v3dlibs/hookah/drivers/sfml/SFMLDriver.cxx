#include "../../Hookah.h"
#include "SFMLWindow.h"
#include "SFMLKeyboard.h"
#include "SFMLMouse.h"

#include <boost/shared_ptr.hpp>

boost::shared_ptr<Hookah::Window> Hookah::Create3DWindow(unsigned width, unsigned int height)
{
	boost::shared_ptr<Hookah::Window> win;
	try
	{
		std::string caption("");
		win.reset(new Hookah::SFMLWindow(width, height, caption));
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
		dev.reset(new Hookah::SFMLKeyboard);
	}
	else if (device == "mouse")
	{
		dev.reset(new Hookah::SFMLMouse);
	}

	return dev;
}

