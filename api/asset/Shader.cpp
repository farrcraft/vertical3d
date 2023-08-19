/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Shader.h"

namespace v3d::asset {
    /**
     **/
    Shader::Shader(const std::string& name, Type t, boost::shared_ptr<v3d::gl::Shader> shader) :
        Asset(name, t),
        shader_(shader) {
    }

    /**
     **/
    boost::shared_ptr<v3d::gl::Shader> Shader::shader() {
        return shader_;
    }

};  // namespace v3d::asset
