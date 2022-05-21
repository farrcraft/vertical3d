/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <map>
#include <string>

#include "Command.h"
#include "Event.h"

namespace v3d::command {

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

};