/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "EventInfo.h"

#include <boost/shared_ptr.hpp>
#include <vector>
#include <list>

namespace v3d::event
{

	class EventListener;

	class EventEmitter
	{
		public:
			void on(const EventInfo & ev, boost::shared_ptr<EventListener> listener);
			void addListener(const EventInfo & ev, boost::shared_ptr<EventListener> listener);
			void removeListener(const EventInfo & ev, boost::shared_ptr<EventListener> listener);
			void once(const EventInfo & ev, boost::shared_ptr<EventListener> listener);

			void emit(EventInfo e);

		private:
			std::list< std::pair< EventInfo, std::vector< boost::shared_ptr<EventListener> > > > listeners_;
	};


};