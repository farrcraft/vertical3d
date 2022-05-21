/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/


#include "StateController.h"

using namespace v3d::command;

StateController::StateController()
{
}

StateController::~StateController()
{
}

void StateController::add(boost::shared_ptr<CommandTable> state)
{
	_states.push_back(state);
}

bool StateController::remove(const std::string & name)
{
	std::vector< boost::shared_ptr<CommandTable> >::iterator iter = _states.begin();
	for (; iter != _states.end(); iter++)
	{
		if ((*iter)->name() == name)
		{
			_states.erase(iter);
			return true;
		}
	}
	return false;
}

boost::shared_ptr<CommandTable> StateController::get(const std::string & name) const
{
	boost::shared_ptr<CommandTable> state;
	std::vector< boost::shared_ptr<CommandTable> >::const_iterator iter = _states.begin();
	for (; iter != _states.end(); iter++)
	{
		if ((*iter)->name() == name)
		{
			state = (*iter);
			break;
		}
	}
	return state;
}

bool StateController::push(const std::string & name)
{
	std::vector< boost::shared_ptr<CommandTable> >::iterator iter = _states.begin();
	for (; iter != _states.end(); iter++)
	{
		if ((*iter)->name() == name)
		{
			_stack.push_front(*iter);
			return true;
		}
	}
	return false;
}

bool StateController::pop(void)
{
	if (_stack.size() > 0)
	{
		_stack.erase(_stack.begin());
		return true;
	}
	return false;
}
