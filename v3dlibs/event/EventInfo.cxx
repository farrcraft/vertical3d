/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "EventInfo.h"

using namespace v3d::event;

EventInfo::EventInfo(const std::string & name, MatchState match) : name_(name), match_(match)
{
}

EventInfo::EventInfo(const std::string & name, MatchState match, const std::string & data) : name_(name), match_(match), data_(data)
{
}

std::string EventInfo::name() const
{
	return name_;
}

std::string EventInfo::data() const
{
	return data_;
}

EventInfo::MatchState EventInfo::match() const
{
	return match_;
}

bool EventInfo::operator == (const EventInfo & e) const
{
	if (e.name_ != name_ || e.match_ == MATCH_STATE_NONE || match_ == MATCH_STATE_NONE)
		return false;
	if (e.match_ == MATCH_STATE_ANY || match_ == MATCH_STATE_ANY || e.match_ == match_)
		return true;
	return false;
}
