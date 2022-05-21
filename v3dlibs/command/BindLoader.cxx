/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "BindLoader.h"

#include <boost/foreach.hpp>
#include <boost/log/trivial.hpp>

using namespace v3d::command;

bool utility::load_binds(const boost::property_tree::ptree & tree, CommandDirectory * directory)
{
	BOOST_LOG_TRIVIAL(debug) << "utility::load_binds - looking for commands to bind...";
	// <bind event="w" scope="pong" command="leftPaddleUp" param="value" catch="any" />
	std::string event_name, command, val, scope, param;
	v3d::event::EventInfo::MatchState flag = v3d::event::EventInfo::MATCH_STATE_NONE;
	BOOST_FOREACH(boost::property_tree::ptree::value_type const & v, tree.get_child("config.keys"))
	{
		if (v.first == "bind")
		{
			event_name 	= v.second.get<std::string>("<xmlattr>.event");
			command 	= v.second.get<std::string>("<xmlattr>.command");
			scope 		= v.second.get<std::string>("<xmlattr>.scope", "");
			val 		= v.second.get<std::string>("<xmlattr>.catch");
			param 		= v.second.get<std::string>("<xmlattr>.param", "");

			BOOST_LOG_TRIVIAL(debug) << "utility::load_binds - binding event [" << event_name << "] to command [" << command
				<< "] in scope [" << scope << "] catch [" << val << "] param [" << param << "]";

			if (val == "on")
			{
				flag = v3d::event::EventInfo::MATCH_STATE_ON;
			}
			else if (val == "off")
			{
				flag = v3d::event::EventInfo::MATCH_STATE_OFF;
			}
			else if (val == "any")
			{
				flag = v3d::event::EventInfo::MATCH_STATE_ANY;
			}

			directory->bind(v3d::event::EventInfo(event_name, flag),
							CommandInfo(command, scope), 
							param);
		}
	}
	return true;
}
