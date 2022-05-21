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
#ifndef INCLUDED_V3D_TOOL_H
#define INCLUDED_V3D_TOOL_H

#include "Command.h"

namespace v3D
{

	/*
		commands are not interactive
		a single command class can implement multiple commands

		tools are interactive
		a tool class implements a single tool
		tools can listen for events

		derived tools should call the two methods:
			Project::instance().activateTool(this);
			Project::instance().deactivateTool();

		if a tool is complex or has much state (private member data)
		then it may be better to have a separate command class that
		creates the tool on exec().
	*/
	class Tool : public Command
	{
		public:
			Tool();
			virtual ~Tool();

			virtual bool motion(int x, int y);
			virtual bool button(unsigned int num, bool press, int x, int y);

			virtual bool draw(void);
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_TOOL_H
