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
#include "CommandDirectory.h"

using namespace v3D;

CommandDirectory::CommandDirectory()
{
}

CommandDirectory::~CommandDirectory()
{

}

CommandDirectory & CommandDirectory::instance(void)
{
	static CommandDirectory instance;
	return instance;
}

bool CommandDirectory::connect(Command * cmd, const std::string & name, std::string ns)
{
	if (ns.size() == 0)
		ns = "__global__";
	std::map<std::string, CommandTable>::iterator it = _tables.find(ns);
	if (it == _tables.end())
	{
		CommandTable table;
		_tables[ns] = table;
	}
	return _tables[ns].connect(name, cmd);
}

bool CommandDirectory::dispatch(const std::string & cmd, std::string ns) const
{
	if (ns.size() == 0)
		ns = "__global__";
	std::map<std::string, CommandTable>::const_iterator it = _tables.find(ns);
	if (it == _tables.end())
		return false;
	return (*it).second.dispatch(cmd);
}

bool CommandDirectory::bind(const std::string & cmd, const Event & e, std::string ns)
{
	if (ns.size() == 0)
		ns = "__global__";
	std::map<std::string, CommandTable>::iterator it = _tables.find(ns);
	if (it == _tables.end())
		return false;
	return _tables[ns].bind(cmd, e);
}

bool CommandDirectory::exec(const Event & e)
{
	std::map<std::string, CommandTable>::iterator it = _tables.begin();
	for (; it != _tables.end(); it++)
	{
		if ((*it).second.exec(e))
			return true;
	}
	return false;
}

