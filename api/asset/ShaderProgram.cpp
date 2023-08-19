/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "ShaderProgram.h"

namespace v3d::asset {
    /**
     **/
    ShaderProgram::ShaderProgram(const std::string& name, Type t, boost::shared_ptr<v3d::gl::Program> program) :
        Asset(name, t),
        program_(program) {
    }

    /**
     **/
    boost::shared_ptr<v3d::gl::Program> ShaderProgram::program() {
        return program_;
    }

};  // namespace v3d::asset
