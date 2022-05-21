/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "CommandDirectory.h"

#include <cassert>
#include <algorithm>

using namespace v3d::command;

Bind CommandDirectory::lookup(const std::string & cmd) const
{
	CommandInfo info(cmd);
	std::vector<Bind>::const_iterator iter = binds_.begin();
	for (; iter != binds_.end(); ++iter)
	{
		if (iter->command() == info)
			return (*iter);
	}
	Bind empty_bind(v3d::event::EventInfo("", v3d::event::EventInfo::MATCH_STATE_ANY), CommandInfo("", ""), "");
	return empty_bind;
}

void CommandDirectory::bind(const v3d::event::EventInfo & event, const CommandInfo & command, const std::string & param)
{
	Bind the_bind(event, command, param);
	binds_.push_back(the_bind);
}

void CommandDirectory::add(boost::shared_ptr<Command> cmd)
{
	commands_.push_back(cmd);
}

void CommandDirectory::add(const std::string & name, const std::string & scope, 
						  const Command::CommandHandlerType::slot_function_type & slot)
{
	boost::shared_ptr<v3D::Command> cmd(new v3D::Command(name, scope, slot));
	add(cmd);
}

bool CommandDirectory::remove(const std::string & name, const std::string & scope)
{
	CommandInfo info(name, scope);
	std::vector< boost::shared_ptr<Command> >::iterator iter = commands_.begin();
	for (; iter != commands_.end(); ++iter)
	{
		if (*(*iter) == info)
		{
			commands_.erase(iter);
			return true;
		}
	}
	return false;
}

bool CommandDirectory::exec(const CommandInfo & cmd, const std::string & param)
{
	std::vector< boost::shared_ptr<Command> >::iterator iter = commands_.begin();
	for (; iter != commands_.end(); ++iter)
	{
		if (*(iter->get()) == cmd)
		{
			// call the command handler
			(*iter)->exec(param);
			return true;
		}
	}
	return false;
}

void CommandDirectory::notify(const v3d::event::EventInfo & e)
{
	// find a bind that matches the event
	std::vector<Bind>::iterator result = binds_.begin();
	for (; result != binds_.end(); ++result)
	{
		if (result->event() == e)
		{
			exec(result->command(), result->param());
		}
	}
}
