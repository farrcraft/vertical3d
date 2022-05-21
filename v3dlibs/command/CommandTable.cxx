/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "CommandTable.h"

using namespace v3d::command;

CommandTable::CommandTable()
{
}

CommandTable::~CommandTable()
{
}

bool CommandTable::connect(const std::string & name, Command * cmd)
{
	_commands[name] = cmd;
	return true;
}

bool CommandTable::dispatch(const std::string & cmd) const
{

	std::map<std::string, Command*>::const_iterator it = _commands.find(cmd);
	if (it == _commands.end())
		return false;
	//_log.level(slug::log_base::ll_info) << "dispatching " << cmd << "\n";
	return (*it).second->exec(cmd);
}

bool CommandTable::bind(const std::string & cmd, const Event & e)
{
	_binds[cmd] = e;
	return true;
}

bool CommandTable::exec(const Event & e)
{
	//_log.level(slug::log_base::ll_info) << "exec " << e.name() << "\n";

	std::map<std::string, Event>::const_iterator it = _binds.begin();
	for (; it != _binds.end(); it++)
	{
		if ((*it).second.name() == e.name())
		{
			return dispatch((*it).first);
		}
	}
	return false;
}
