/**
 * Stark Game Engine Utility Library
 *
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#include "ProgramFactory.h"
#include "AssetLoader.h"

#include <vertical3d/gl/Shader.h>

ProgramFactory::ProgramFactory(boost::shared_ptr<AssetLoader> loader)
	: loader_(loader)
{
}

boost::shared_ptr<v3D::Program> ProgramFactory::create(unsigned int shaderTypes, const std::string & name)
{
	std::vector<boost::shared_ptr<v3D::Shader>> theShaders;
	boost::shared_ptr<v3D::Program> program;
	std::string script;
	boost::shared_ptr<v3D::Shader> shader;

	if (shaderTypes & v3D::Shader::SHADER_TYPE_VERTEX)
	{
		std::string filename = name + std::string(".vert");
		script = loader_->load(filename);
		shader.reset(new v3D::Shader(v3D::Shader::SHADER_TYPE_VERTEX, script));
		theShaders.push_back(shader);
	}

	if (shaderTypes & v3D::Shader::SHADER_TYPE_FRAGMENT)
	{
		std::string filename = name + std::string(".frag");
		script = loader_->load(filename);
		shader.reset(new v3D::Shader( v3D::Shader::SHADER_TYPE_FRAGMENT, script));
		theShaders.push_back(shader);
	}

	program.reset(new v3D::Program(theShaders));

	return program;
}
