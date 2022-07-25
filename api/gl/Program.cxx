/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Program.h"

#include <GL/glew.h>

#include <stdexcept>

#include "Shader.h"

namespace v3d::gl {
    Program::Program(std::vector<boost::shared_ptr<Shader>>& theShaders) :
        enabled_(false) {
        shaders(theShaders);
    }

    /**
     **/
    void Program::shaders(std::vector<boost::shared_ptr<Shader>>& theShaders) {
        id_ = glCreateProgram();

        for (std::vector<boost::shared_ptr<Shader>>::size_type i = 0; i != theShaders.size(); i++) {
            theShaders[i]->attach(id_);
        }

        glLinkProgram(id_);

        GLint status;
        glGetProgramiv(id_, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            GLint infoLogLength;
            glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &infoLogLength);

            GLchar* infoLog = new GLchar[infoLogLength + 1];
            glGetProgramInfoLog(id_, infoLogLength, NULL, infoLog);
            std::string msg = std::string("Shader Program Linker failure: ") + std::string(infoLog);
            delete[] infoLog;

            throw std::runtime_error(msg);
        }

        for (std::vector<boost::shared_ptr<Shader>>::size_type i = 0; i != theShaders.size(); i++) {
            theShaders[i]->detach(id_);
        }
    }

    /**
     **/
    void Program::enable() {
        if (!enabled_) {
            glUseProgram(id_);
            enabled_ = true;
        }
    }

    /**
     **/
    void Program::disable() {
        if (enabled_) {
            glUseProgram(0);
            enabled_ = false;
        }
    }

    /**
     **/
    unsigned int Program::uniform(const std::string& name) {
        if (uniforms_.find(name) == uniforms_.end()) {
            if (!enabled_) {
                enable();
            }
            uniforms_[name] = glGetUniformLocation(id_, name.c_str());
        }
        return uniforms_[name];
    }

};  // namespace v3d::gl
