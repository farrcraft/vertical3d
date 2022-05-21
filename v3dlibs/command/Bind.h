/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "CommandInfo.h"
#include "../event/EventInfo.h"

namespace v3d::command
{
	/**
	 * A Bind holds a connection between an event and a command using CommandInfo and 
	 * EventInfo objects. Each Bind object can also store a paramater string which will 
	 * be passed to the command handler when the event is fired. This allows the same
	 * command to be bound to multiple events.
	 * A binding specification in XML format:
	 *	\<bind key="escape" command="toggleMenu" scope="luxa" param="game-menu" catch="on" />
	 */
	class Bind
	{
		public:
			/**
			 * Constructor
			 * @param evnt the EventInfo object describing the bound event
			 * @param command the CommandInfo object describing the bound command
			 * @param param the parameter string to pass to the Command callback
			 */
			Bind(const v3d::event::EventInfo & evnt, const CommandInfo & command, const std::string & param);

			/**
			 * Comparison operator. Only the internal EventInfo object is compared against.
			 * @param e the EventInfo object describing the event to match this bind against.
			 * @return whether the event matches this bind
			 */
			bool operator == (const v3d::event::EventInfo & e) const;

			/**
			 * Comparison operator. Full object comparison.
			 * @param b the Bind object to test against
			 * @return whether the event matches this bind
			 */
			bool operator == (const Bind &b) const;

			/**
			 * Get the Bind's CommandInfo
			 * @return a copy of the CommandInfo
			 */
			CommandInfo command() const;
			/**
			 * Get the Bind's EventInfo
			 * @return a copy of the EventInfo
			 */
			v3d::event::EventInfo event() const;
			/**
			 * Get the Command callback's parameter string
			 * @return the parameter string
			 */
			std::string param() const;

		private:
			CommandInfo command_;
			v3d::event::EventInfo event_;
			std::string param_;
	};

};
