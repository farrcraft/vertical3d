/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "Event.h"

using namespace v3d::command;

Event::Event() : _catchflag(MATCH_STATE_ANY)
{
}

Event::Event(const std::string & name, std::string ns, MatchState flag)
{
	if (ns.size() == 0)
		ns = "__global__";

	_namespace = ns;
	_name = name;
	_catchflag = flag;
}

Event::~Event()
{
}

std::string Event::name(void) const
{
	return _name;
}

std::string Event::ns(void) const
{
	return _namespace;
}

Event::MatchState Event::flag(void) const
{
	return _catchflag;
}

void Event::name(const std::string & n)
{
	_name = n;
}

void Event::ns(const std::string & n)
{
	_namespace = n;
}

void Event::flag(MatchState f)
{
	_catchflag = f;
}
