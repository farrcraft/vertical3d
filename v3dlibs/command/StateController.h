/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "CommandTable.h"
#include <boost/shared_ptr.hpp>
#include <deque>

namespace v3d::command
{
	class StateController
	{
		public:
			StateController();
			virtual ~StateController();

			void add(boost::shared_ptr<CommandTable> state);
			boost::shared_ptr<CommandTable> get(const std::string & name) const;
			bool remove(const std::string & name);
			bool push(const std::string & name);
			bool pop(void);

		private:
			std::vector<boost::shared_ptr<CommandTable> > _states;
			std::deque<boost::shared_ptr<CommandTable> > _stack;
	};

};
