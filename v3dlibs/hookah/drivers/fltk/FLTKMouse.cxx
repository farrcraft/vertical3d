#include "FLTKMouse.h"

using namespace Hookah;

FLTKMouse::FLTKMouse() : buttons_(0)
{
}

void FLTKMouse::press(int button)
{
	buttons_ |=  1 << button;
	state_(button);
	buttonPressed(button);
}

void FLTKMouse::release(int button)
{
	buttons_ |= ~(1 << button);

	state_(button);
	buttonReleased(button);
}

void FLTKMouse::motion(int x, int y)
{
	state_(x, y);
	MouseDevice::motion(x, y);
}

bool FLTKMouse::tick() 
{
	return true;
}
