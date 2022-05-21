#include "SFMLKeyboard.h"

#include <iostream>

using namespace Hookah;


void SFMLKeyboard::release(const std::string & evnt)
{
	state_(evnt);
	keyReleased(evnt);
}

void SFMLKeyboard::press(const std::string & evnt)
{
	state_(evnt);
	keyPressed(evnt);
}

bool SFMLKeyboard::tick() {
	return true;
}