/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "ProgramFactory.h"
#include "AssetLoader.h"

#include "../../api/gl/Shader.h"

#include <boost/make_shared.hpp>

ProgramFactory::ProgramFactory(boost::shared_ptr<AssetLoader> loader)
	: loader_(loader)
{
}

boost::shared_ptr<v3d::gl::Program> ProgramFactory::create(unsigned int shaderTypes, const std::string & name)
{
	std::vector<boost::shared_ptr<v3d::gl::Shader>> theShaders;
	boost::shared_ptr<v3d::gl::Program> program;
	std::string script;
	boost::shared_ptr<v3d::gl::Shader> shader;

	if (shaderTypes & v3d::gl::Shader::SHADER_TYPE_VERTEX)
	{
		std::string filename = name + std::string(".vert");
		script = loader_->load(filename);
		shader.reset(new v3d::gl::Shader(v3d::gl::Shader::SHADER_TYPE_VERTEX, script));
		theShaders.push_back(shader);
	}

	if (shaderTypes & v3d::gl::Shader::SHADER_TYPE_FRAGMENT)
	{
		std::string filename = name + std::string(".frag");
		script = loader_->load(filename);
		shader.reset(new v3d::gl::Shader( v3d::gl::Shader::SHADER_TYPE_FRAGMENT, script));
		theShaders.push_back(shader);
	}

	program = boost::make_shared<v3d::gl::Program>(theShaders);

	return program;
}
