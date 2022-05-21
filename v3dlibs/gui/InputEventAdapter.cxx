/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "InputEventAdapter.h"

#include <boost/lexical_cast.hpp>

using namespace v3d::input;

InputEventAdapter::InputEventAdapter(boost::shared_ptr<KeyboardDevice> kb, boost::shared_ptr<MouseDevice> mouse)
{
	if(kb)
	{
		kb->addEventListener(this, "event_adapter");
	}
	if (mouse)
	{
		mouse->addEventListener(this, "event_adapter");
	}
}

void InputEventAdapter::connect(EventListener * target)
{
	targets_.push_back(target);
}

void InputEventAdapter::notify(const v3d::event::EventInfo & info)
{
	std::vector<EventListener * >::const_iterator iter = targets_.begin();
	for (; iter != targets_.end(); iter++)
	{
		(*iter)->notify(info);
	}
}

void InputEventAdapter::motion(unsigned int x, unsigned int y)
{
	std::string name = "mouse::";
	name += "motion";
	v3d::event::EventInfo e(name, v3d::event::EventInfo::MATCH_STATE_ANY);
	notify(e);
}

void InputEventAdapter::buttonPressed(unsigned int button)
{
	std::string name = "mouse::";
	name += "button_";
	name += boost::lexical_cast<std::string>(button);
	v3d::event::EventInfo e(name, v3d::event::EventInfo::MATCH_STATE_ON);
	notify(e);
}

void InputEventAdapter::buttonReleased(unsigned int button)
{
	std::string name = "mouse::";
	name += "button_";
	name += boost::lexical_cast<std::string>(button);
	v3d::event::EventInfo e(name, v3d::event::EventInfo::MATCH_STATE_OFF);
	notify(e);
}

void InputEventAdapter::keyPressed(const std::string & key)
{
	v3d::event::EventInfo e(key, v3d::event::EventInfo::MATCH_STATE_ON);
	notify(e);
}

void InputEventAdapter::keyReleased(const std::string & key)
{
	v3d::event::EventInfo e(key, v3d::event::EventInfo::MATCH_STATE_OFF);
	notify(e);
}
