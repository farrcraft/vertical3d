/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */
#include "KeyboardDevice.h"
#include "KeyboardEventListener.h"

#include <algorithm>

using namespace v3D;

bool KeyboardDevice::KeyState::pressed(const std::string & c) const
{
	std::vector<std::string>::const_iterator iter = std::find(keys_.begin(), keys_.end(), c);
	if (iter != keys_.end())
	{
		return true;
	}
	return false;
}

bool KeyboardDevice::KeyState::operator() (const std::string & c)
{
	std::vector<std::string>::const_iterator iter = std::find(keys_.begin(), keys_.end(), c);
	bool pressed = true;
	if (iter != keys_.end())
	{
		keys_.erase(iter);
		pressed = false;
	}
	else
	{
		keys_.push_back(c);
	}
	return pressed;
}

void KeyboardDevice::addEventListener(KeyboardEventListener * listener, const std::string & name)
{
	listeners_[name] = listener;
}

bool KeyboardDevice::removeEventListener(const std::string & name)
{	
	std::map<std::string, KeyboardEventListener *>::iterator iter = listeners_.find(name);
	if (iter != listeners_.end())
	{
		listeners_.erase(iter);
		return true;
	}
	return false;
}

void KeyboardDevice::keyPressed(const std::string & key)
{
	std::map<std::string, KeyboardEventListener *>::iterator iter = listeners_.begin();
	for (; iter != listeners_.end(); ++iter)
	{
		(*iter).second->keyPressed(key);
	}
}

void KeyboardDevice::keyReleased(const std::string & key)
{
	std::map<std::string, KeyboardEventListener *>::iterator iter = listeners_.begin();
	for (; iter != listeners_.end(); ++iter)
	{
		(*iter).second->keyReleased(key);
	}
}
