/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shader.h"

#include <GL/glew.h>
#include <boost/log/trivial.hpp>

#include <fstream>
#include <iostream>

using namespace v3d::gl;

Shader::Shader(ShaderType type, const std::string & shader) : 
	type_(type), 
	id_(0)
{
	GLenum shaderType;
	switch (type_)
	{
		case SHADER_TYPE_VERTEX:
			shaderType = GL_VERTEX_SHADER;
			break;
		case SHADER_TYPE_FRAGMENT:
			shaderType = GL_FRAGMENT_SHADER;
			break;
		default:
			shaderType = GL_VERTEX_SHADER;
			break;
	}
	id_ = glCreateShader(shaderType);
	const char *data = shader.c_str();
	glShaderSource(id_, 1, &data, NULL);

	glCompileShader(id_);

	GLint status;
	glGetShaderiv(id_, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetShaderiv(id_, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *infoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(id_, infoLogLength, NULL, infoLog);

		std::string shaderTypeName;
		switch(shaderType)
		{
			case GL_VERTEX_SHADER: 
				shaderTypeName = "vertex"; 
				break;
			case GL_GEOMETRY_SHADER: 
				shaderTypeName = "geometry"; 
				break;
			case GL_FRAGMENT_SHADER: 
				shaderTypeName = "fragment"; 
				break;
		}

		std::string msg = std::string("Compile failure in ") + shaderTypeName + std::string(" shader: ") + std::string(infoLog);
		delete[] infoLog;
		BOOST_LOG_TRIVIAL(error) << msg;
		throw std::runtime_error(msg);
	}
}

Shader::~Shader()
{
	glDeleteShader(id_);
}

void Shader::attach(unsigned int program)
{
	glAttachShader(program, id_);
}

void Shader::detach(unsigned int program)
{
	glDetachShader(program, id_);
}

