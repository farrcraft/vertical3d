#include "SFMLMouse.h"

using namespace Hookah;

SFMLMouse::SFMLMouse() : buttons_(0)
{
}
void SFMLMouse::press(int button)
{
	buttons_ |=  1 << button;
	state_(button);
	buttonPressed(button);
}

void SFMLMouse::release(int button)
{
	buttons_ |= ~(1 << button);

	state_(button);
	buttonReleased(button);
}

void SFMLMouse::motion(int x, int y)
{
	state_(x, y);
	MouseDevice::motion(x, y);
}

bool SFMLMouse::tick()
{
	return true;
}
