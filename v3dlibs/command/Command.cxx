/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "Command.h"

using namespace v3d::command;

Command::Command(const std::string & name, const std::string & scope, const CommandHandlerType::slot_function_type & slot) 
	: info_(name, scope),
	blocker_(connection_)
{
	connection_ = handler_.connect(slot);
}

bool Command::operator == (const CommandInfo & cmd) const
{
	if (cmd == info_)
		return true;
	return false;
}

bool Command::exec(const std::string & param)
{
	bool result = *handler_(info_, param);
	return result;
}

void Command::disconnect()
{
	if (!handler_.empty())
		connection_.disconnect();
}

void Command::enable()
{
	if (!handler_.empty())
		blocker_.unblock();
}

void Command::disable()
{
	if (!handler_.empty())
		blocker_.block();
}
