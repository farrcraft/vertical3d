/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "EventEmitter.h"
#include "EventListener.h"

using namespace v3d::event;

void EventEmitter::on(const EventInfo & ev, boost::shared_ptr<EventListener> listener)
{
	addListener(ev, listener);
}

void EventEmitter::addListener(const EventInfo & ev, boost::shared_ptr<EventListener> listener)
{
	std::list< std::pair<EventInfo, std::vector< boost::shared_ptr<EventListener> > > >::iterator it;
	for (it = listeners_.begin(); it != listeners_.end(); ++it)
	{
		if (it->first == ev)
		{
			it->second.push_back(listener);
			return;
		}
	}
	std::vector<boost::shared_ptr<EventListener> > listeners;
	listeners.push_back(listener);
	std::pair<EventInfo, std::vector<boost::shared_ptr<EventListener> > > pair(ev, listeners);
	listeners_.push_back(pair);
}

void EventEmitter::removeListener(const EventInfo & ev, boost::shared_ptr<EventListener> listener)
{
	std::list< std::pair<EventInfo, std::vector< boost::shared_ptr<EventListener> > > >::iterator it;
	for (it = listeners_.begin(); it != listeners_.end(); ++it)
	{
		if (it->first == ev)
		{
			std::vector< boost::shared_ptr<EventListener> >::iterator et;
			for (et = it->second.begin(); et != it->second.end(); ++et)
			{
				if (*et == listener)
				{
					it->second.erase(et);
					return;
				}
			}
		}
	}
}

void EventEmitter::once(const EventInfo & ev, boost::shared_ptr<EventListener> listener)
{
}

void EventEmitter::emit(EventInfo e)
{
	std::list< std::pair<EventInfo, std::vector< boost::shared_ptr<EventListener> > > >::iterator it;
	for (it = listeners_.begin(); it != listeners_.end(); ++it)
	{
		if (it->first == e)
		{
			std::vector< boost::shared_ptr<EventListener> >::iterator et;
			for (et = it->second.begin(); et != it->second.end(); ++et)
			{
				(*et)->notify(e);
			}
		}
	}
}

