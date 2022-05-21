#include "StateMachine.h"

#include <iostream>

using namespace FSM;

StateMachine::StateMachine() : _current(0), _next(0)
{
}

void StateMachine::addState(const std::string & name, StateBase * state)
{
	state->addref();
	_states[name] = state;
}

StateBase * StateMachine::getState(const std::string & name)
{
	if (name.empty())
		return 0;
	value_type::iterator iter = _states.find(name);
	StateBase * state = 0;
	if (iter != _states.end())
		state = _states[name];
	return state;
}

StateMachine::~StateMachine()
{
	value_type::iterator it = _states.begin();
	for (; it != _states.end(); it++)
	{
		StateBase * state = it->second;
		if (state->release() == 0)
		{
			delete state;
		}
	}
}

std::string StateMachine::activeStateName() const
{
	return _activeStateName;
}

void StateMachine::run(const std::string & start)
{
	value_type::iterator it = _states.find(start);
	if (it == _states.end())
	{
		std::cout << "initial state '" << start << "' not found!" << std::endl;
		return;
	}
	_activeStateName = start;
	_current = it->second;

/*
	std::string nextStateName;
	while (_current)
	{
		std::cout << "running state: " << _activeStateName << std::endl;
		// run the current state
		nextStateName = _current->run();
		// get the next state
		_next = getState(nextStateName);
		// record history 
		_history.push_back(_activeStateName);
		// prepare to enter the next state
		_activeStateName = nextStateName;
		_current = _next;
		_next = 0;
	}
*/
	_history.push_back(_activeStateName);
	_current->run();
}

bool StateMachine::callState(const std::string & name)
{
	StateBase * child = getState(name);

//	std::cout << "called child state: " << name << std::endl;

	if (!child)
	{
		std::cout << "child state '" << name << "' didn't exist!" << std::endl;
		return false;
	}

	std::string old_state = _activeStateName;
	_activeStateName = name;
	_history.push_back(name);
	_stack.push_back(name);

	bool result = child->run();

	_stack.pop_back();
	_activeStateName = old_state;

	return result;
}

std::string StateMachine::previousState()
{
	if (_history.empty())
		return "";

	return _history.back();
}

bool StateMachine::history(const std::string & name)
{
	if (_history.empty())
		return false;

	std::deque<std::string>::iterator iter = _history.begin();
	for (; iter != _history.end(); iter++)
	{
		if ((*iter) == name)
			return true;
	}
	return false;
}
/*
void StateMachine::push(void)
{
	std::cout << "pushed state: " << _activeStateName << std::endl;
	_stack.push_back(_activeStateName);
}

std::string StateMachine::pop(void)
{
	if (!_stack.empty())
	{
		std::cout << "popping state off stack: " << _stack.back() << std::endl;
		_stack.pop_back();
	}
	else
	{
		std::cout << "tried to pop empty stack!" << std::endl;
	}

	if (_stack.empty())
	{
		std::cout << "popped to empty stack" << std::endl;
		return "";
	}

	std::cout << "popped to stack state: " << _stack.back() << std::endl;

	return _stack.back();
}

bool StateMachine::marked(const std::string & name)
{
	std::deque<std::string>::iterator iter = _stack.begin();
	for (; iter != _stack.end(); iter++)
	{
		if ((*iter) == name)
			return true;
	}
	return false;
}
*/
