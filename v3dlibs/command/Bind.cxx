/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "Bind.h"

using namespace v3d::command;

Bind::Bind(const v3d::event::EventInfo & evnt, const CommandInfo & command, const std::string & param) :
	command_(command), 
	event_(evnt), 
	param_(param)
{
}


bool Bind::operator == (const v3d::event::EventInfo & e) const
{
	if (event_ == e)
		return true;
	return false;
}


bool Bind::operator == (const Bind & b) const
{
	if (event_ == b.event() &&
		command_ == b.command() &&
		param_ == b.param()) 
	{
		return true;
	}
	return false;
}


CommandInfo Bind::command() const
{
	return command_;
}

v3d::event::EventInfo Bind::event() const
{
	return event_;
}

std::string Bind::param() const
{
	return param_;
}
