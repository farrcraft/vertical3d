#include "InputEventAdapter.h"

#include <boost/lexical_cast.hpp>

using namespace v3D;

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

void InputEventAdapter::notify(const EventInfo & info)
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
	EventInfo e(name, EventInfo::MATCH_STATE_ANY);
	notify(e);
}

void InputEventAdapter::buttonPressed(unsigned int button)
{
	std::string name = "mouse::";
	name += "button_";
	name += boost::lexical_cast<std::string>(button);
	EventInfo e(name, EventInfo::MATCH_STATE_ON);
	notify(e);
}

void InputEventAdapter::buttonReleased(unsigned int button)
{
	std::string name = "mouse::";
	name += "button_";
	name += boost::lexical_cast<std::string>(button);
	EventInfo e(name, EventInfo::MATCH_STATE_OFF);
	notify(e);
}

void InputEventAdapter::keyPressed(const std::string & key)
{
	EventInfo e(key, EventInfo::MATCH_STATE_ON);
	notify(e);
}

void InputEventAdapter::keyReleased(const std::string & key)
{
	EventInfo e(key, EventInfo::MATCH_STATE_OFF);
	notify(e);
}
