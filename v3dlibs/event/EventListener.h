/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "EventInfo.h"

namespace v3d::event
{

	/** 
	 * An Abstract Base Class for any derived class which wants to listen and
	 * be notified of events.
	 */
	class EventListener
	{
		public:
			virtual ~EventListener() { }

			/**
			 * Event notification method.
			 * @param e the event that has occurred.
			 */
			virtual void notify(const EventInfo & e) = 0;

	};

};
