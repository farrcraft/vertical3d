/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "CommandTable.h"

using namespace v3d::command;

/**
 **/
CommandTable::CommandTable(const std::string &name) : name_(name) {
}

/**
 **/
std::string_view CommandTable::name() const {
	return name_;
}

/**
 **/
bool CommandTable::connect(const std::string & name, Command * cmd) {
	commands_[name] = cmd;
	return true;
}

/**
 **/
bool CommandTable::dispatch(const std::string & cmd) const {

	std::map<std::string, Command*>::const_iterator it = commands_.find(cmd);
	if (it == commands_.end()) {
		return false;
	}
	//_log.level(slug::log_base::ll_info) << "dispatching " << cmd << "\n";
	return (*it).second->exec(cmd);
}

/**
 **/
bool CommandTable::bind(const std::string & cmd, const Event & e) {
	binds_[cmd] = e;
	return true;
}

/**
 **/
bool CommandTable::exec(const Event & e) {
	//_log.level(slug::log_base::ll_info) << "exec " << e.name() << "\n";

	std::map<std::string, Event>::const_iterator it = binds_.begin();
	for (; it != binds_.end(); it++) {
		if ((*it).second.name() == e.name()) {
			return dispatch((*it).first);
		}
	}
	return false;
}
