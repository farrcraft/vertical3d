/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "CommandInfo.h"

#include <boost/signals2.hpp>

namespace v3d::command
{

	/**
	 * An application Command object. The Command consists of its CommandInfo object and a command callback.
	 * 
	 */
	class Command
	{
		public:
			/**
			 * Command callbacks should be in the form:
			 *	bool callback(const CommandInfo & cmd, const std::string & param);
			 * 
			 */
			typedef boost::signals2::signal<bool (const CommandInfo & , const std::string &) > CommandHandlerType;
			/**
			 * Constructor
			 * @param name the name of the command
			 * @param scope the scope of the command (e.g. application or subsystem name)
			 * @param slot the command callback method
			 */
			Command(const std::string & name, const std::string & scope, const CommandHandlerType::slot_function_type & slot);
			/**
			 * Comparison operator. Compares a CommandInfo object with just the Command's internal CommandInfo object.
			 * @param cmd the CommandInfo object to compare against this command.
			 */
			bool operator == (const CommandInfo & cmd) const;
			/**
			 * Call the Command's callback method with the given parameter string.
			 * @param param the callback's parameter string
			 */
			bool exec(const std::string & param);
			/**
			 * Disconnect the command handler from executing
			 */
			void disconnect();

			void enable();
			void disable();

		private:
			CommandHandlerType handler_;
			CommandInfo info_;
			boost::signals2::connection connection_;
			boost::signals2::shared_connection_block blocker_;
	};

};
