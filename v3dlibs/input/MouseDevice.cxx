/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "MouseDevice.h"
#include "MouseEventListener.h"

#include <algorithm>

using namespace v3d::input;

bool MouseDevice::MouseState::pressed(unsigned int button) const
{
	std::vector<unsigned int>::const_iterator iter = std::find(buttons_.begin(), buttons_.end(), button);
	if (iter != buttons_.end())
	{
		return true;
	}
	return false;
}

MouseDevice::MouseState::MotionInfo MouseDevice::MouseState::operator() (unsigned int x, unsigned int y)
{
	MotionInfo tmp;
	tmp.position_[0] = position_.position_[0];
	tmp.position_[1] = position_.position_[1];

	position_.position_[0] = x;
	position_.position_[1] = y;

	return tmp;
}

bool MouseDevice::MouseState::operator() (unsigned int button)
{
	std::vector<unsigned int>::const_iterator iter = std::find(buttons_.begin(), buttons_.end(), button);
	bool pressed = true;
	if (iter != buttons_.end())
	{
		std::remove(buttons_.begin(), buttons_.end(), button);
		pressed = false;
	}
	else
	{
		buttons_.push_back(button);
	}
	return pressed;
}

MouseDevice::MouseState::MotionInfo MouseDevice::MouseState::position() const
{
	return position_;
}

MouseDevice::MouseDevice()
{
}

void MouseDevice::addEventListener(MouseEventListener * listener, const std::string & name)
{
	listeners_[name] = listener;
}

bool MouseDevice::removeEventListener(const std::string & name)
{
	std::map<std::string, MouseEventListener *>::iterator iter = listeners_.find(name);
	if (iter != listeners_.end())
	{
		listeners_.erase(iter);
		return true;
	}
	return false;
}

void MouseDevice::motion(unsigned int x, unsigned int y)
{
	std::map<std::string, MouseEventListener *>::iterator iter = listeners_.begin();
	for (; iter != listeners_.end(); ++iter)
	{
		(*iter).second->motion(x, y);
	}
}

void MouseDevice::buttonPressed(unsigned int button)
{
	std::map<std::string, MouseEventListener *>::iterator iter = listeners_.begin();
	for (; iter != listeners_.end(); ++iter)
	{
		(*iter).second->buttonPressed(button);
	}
}

void MouseDevice::buttonReleased(unsigned int button)
{
	std::map<std::string, MouseEventListener *>::iterator iter = listeners_.begin();
	for (; iter != listeners_.end(); ++iter)
	{
		(*iter).second->buttonReleased(button);
	}
}
