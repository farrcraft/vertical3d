/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <boost/shared_ptr.hpp>
#include <string>

#include "../../api/gl/Program.h"

class AssetLoader;

/**
 * Factory to create new GL Shader Programs
 */
class ProgramFactory
{
	public:
		/**
		 * Construct a new factory
		 *
		 * @param AssetLoader loader a loader used for loading shader files
		 */
		ProgramFactory(boost::shared_ptr<AssetLoader> loader);

		/**
		 * Create a new program
		 *
		 * @param unsigned int shaderTypes bitmask of the types of shaders linked to the program.
		 * @param string name the base filename of the shaders. Path is determined by the loader 
		 *					  and suffix is automatically appended based on the shader type.
		 *
		 * @return Program new shader program with shaders loaded, compiled, and linked.
		 */
		boost::shared_ptr<v3d::gl::Program> create(unsigned int shaderTypes, const std::string & name);

	private:
		boost::shared_ptr<AssetLoader> loader_;
};
