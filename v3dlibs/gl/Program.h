/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <boost/shared_ptr.hpp>

#include <vector>
#include <string>
#include <map>

namespace v3d::gl
{
	class Shader;

	/**
	 * A GLSL shader program
	 */
	class Program
	{
		public:
			/**
			 * Construct a new program a collection of shaders
			 */
			Program(std::vector<boost::shared_ptr<Shader>> & theShaders);
			~Program() = default;

			/**
			 * Start using the program
			 */
			void enable();

			/**
			 * Stop using the program
			 */
			void disable();

			/**
			 * Get the id of a uniform shader program variable
			 *
			 */
			unsigned int uniform(const std::string & name);

		protected:
			void shaders(std::vector<boost::shared_ptr<Shader>> & theShaders);

		private:
			unsigned int id_;
			std::map<std::string, unsigned int> uniforms_;
			bool enabled_;
	};

}; // end namespace v3D