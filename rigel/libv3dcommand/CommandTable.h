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
#ifndef INCLUDED_V3D_COMMANDTABLE_H
#define INCLUDED_V3D_COMMANDTABLE_H

#include <map>
#include <string>

#include "Command.h"
#include "Event.h"

namespace v3D
{

	class CommandTable
	{
		public:
			CommandTable();
			virtual ~CommandTable();

			bool connect(const std::string & name, Command * cmd);
			bool dispatch(const std::string & cmd) const;
			bool bind(const std::string & cmd, const Event & e);
			bool exec(const Event & e);

		private:
			std::map<std::string, Command*>		_commands;
			std::map<std::string, Event>		_binds;
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_COMMANDTABLE_H
