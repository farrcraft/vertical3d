/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "CommandInfo.h"

using namespace v3d::command;

CommandInfo::CommandInfo(const std::string & name, const std::string & scope) : name_(name), scope_(scope)
{
}

CommandInfo::CommandInfo(const std::string & cmd)
{
	std::string::size_type pos = cmd.find("::");
	scope_ = cmd.substr(0, pos);
	if (cmd.length() > pos + 2)
	{
		name_ = cmd.substr(pos + 2);
	}
}

bool CommandInfo::operator == (const CommandInfo & cmd) const
{
	if (cmd.name_ == name_ && cmd.scope_ == scope_)
	{
		return true;
	}
	return false;
}

std::string CommandInfo::name() const
{
	return name_;
}

std::string CommandInfo::scope() const
{
	return scope_;
}
