/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "../event/EventListener.h"
#include "../event/EventInfo.h"
#include "Command.h"
#include "Bind.h"

#include <map>

namespace v3d::command {
	/**
	 * The CommandDirectory class. 
	 *
	 * Definitions:
	 * Commands 
	 *		Defined at the application level. 
	 * Command Info
	 *		Consist of a scope and a name.
	 * Events
	 *		Dispatched by event generating subsystems
	 * EventInfo
	 *		Consist of an event name and flag
	 * Binds
	 *		Consist of CommandInfo, EventInfo, and a param string
	 *
	 *
	 * Functional:
	 * Application level subsystems wanting to provide commands usable in the event system should 
	 * register them with the application's global CommandDirectory object. The subsystem also implements the exec method
	 * and registers CommandInfo's for all of the commands it implements.
	 * Any subsystem which generates events will notify the CommandDirectory that the event has occurred with an 
	 * EventInfo object.
	 *
	 **/
	class CommandDirectory : public v3d::event::EventListener {
		public:
			/**
			 * Bind a command to an event.
			 * @param event an EventInfo object describing the event to be bound
			 * @param command the CommandInfo object describing the command to be bound
			 * @param param the parameter string that will be passed to the command handler
			 *	when the command bound to this event is called.
			 */
			void bind(const v3d::event::EventInfo & event, const CommandInfo & command, const std::string & param);
			/**
			 * Register a new command with the CommandDirectory.
			 * @param cmd a pointer to the Command to be added.
			 */
			void add(boost::shared_ptr<Command> cmd);
			/**
			 * Register a new Command with the CommandDirectory.
			 * @param name the command name
			 * @param scope the command scope
			 * @param slot the command callback
			 */
			void add(const std::string & name, const std::string & scope, const Command::CommandHandlerType::slot_function_type & slot);
			/**
			 * EventListener inheritance. The callbacks for any commands bound to the specified event will be called.
			 * @param e the event that will be dispatched.
			 */
			void notify(const v3d::event::EventInfo & e);
			/**
			 * Remove a Command from the CommandDirectory
			 * @param name the name of the command
			 * @param scope the scope of the command
			 * @return whether the command was successfully removed
			 */
			bool remove(const std::string & name, const std::string & scope);
			/**
			 * Execute a Command
			 * @param cmd a CommandInfo object describing the command to execute
			 * @param param the parameter string to pass to the command handler
			 * @return whether the command was successfully executed
			 */
			bool exec(const CommandInfo & cmd, const std::string & param);

			Bind lookup(const std::string & cmd) const;

		private:
			std::vector<Bind> binds_;
			std::vector< boost::shared_ptr<Command> > commands_;

	};

};
