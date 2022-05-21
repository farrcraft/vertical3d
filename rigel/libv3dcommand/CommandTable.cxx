/*
	Copyright (C) 2001-2004 by Josh Farr
	merkaba_at_quantumfish_dot_com

	This file is part of Vertical|3D.

	Vertical|3D is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Vertical|3D is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Vertical|3D; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "CommandTable.h"

#include <libv3dcore/log/logstream.h>

using namespace v3D;

extern slug::logstream _log;

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
	_log.level(slug::log_base::ll_info) << "dispatching " << cmd << "\n";
	return (*it).second->exec(cmd);
}

bool CommandTable::bind(const std::string & cmd, const Event & e)
{
	_binds[cmd] = e;
	return true;
}

bool CommandTable::exec(const Event & e)
{
	_log.level(slug::log_base::ll_info) << "exec " << e.name() << "\n";

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
