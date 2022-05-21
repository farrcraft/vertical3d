/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace v3d::command
{

	/**
	 * The CommandInfo class stores information describing a command. A command is
	 * composed of a scope and name. The scope separator sequence is "::".
	 */
	class CommandInfo
	{
		public:
			/**
			 * CommandInfo Constructor.
			 * @param name the name of the command
			 * @param scope the scope of the command
			 */
			CommandInfo(const std::string & name, const std::string & scope);
			/**
			 * CommandInfo Constructor. Splits the scope from the command name.
			 * @param cmd a command in the format "scope::command"
			 */
			CommandInfo(const std::string & cmd);

			bool operator == (const CommandInfo & cmd) const;

			/**
			 * Get the name of the command.
			 * @return the command name
			 */
			std::string name() const;
			/**
			 * Get the scope of the command.
			 * @return the command scope
			 */
			std::string scope() const;

		private:
			std::string name_;
			std::string scope_;
	};

}; // end namespace v3D
