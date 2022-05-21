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
#ifndef INCLUDED_V3D_COMMANDDIRECTORY_H
#define INCLUDED_V3D_COMMANDDIRECTORY_H

#include "CommandTable.h"

namespace v3D
{

	/**
		The CommandDirectory class. 
		A global directory of commands.
	 */
	class CommandDirectory
	{
		public:
			/**
				get the global directory instance.
				@return the address of the global directory instance
			 */
			static CommandDirectory & instance(void);
			/**
				register a command in the directory.
				@param cmd a pointer to the command being registered.
				@param name the name of the command.
				@param ns the namespace of the command.
				@return status of registering the command.
			 */
			bool connect(Command * cmd, const std::string & name, std::string ns = "");
			/**
				dispatch a command.
				@param cmd the name of the command to execute.
				@param ns the namespace of the command.
				@return status of executing the named command.
			*/
			bool dispatch(const std::string & cmd, std::string ns = "") const;

			bool bind(const std::string & cmd, const Event & e, std::string ns = "");
			bool exec(const Event & e);

		protected:
			CommandDirectory();
			virtual ~CommandDirectory();

		private:
			std::map<std::string, CommandTable>		_tables;
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_COMMANDDIRECTORY_H
