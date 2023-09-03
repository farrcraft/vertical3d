/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>

#include "Asset.h"
#include "../gl/Program.h"

#include <boost/shared_ptr.hpp>

namespace v3d::asset {
    /**
     **/
    class ShaderProgram : public Asset {
     public:
        /**
         **/
        ShaderProgram(const std::string& name, Type t, boost::shared_ptr<v3d::gl::Program> program);

        /**
         **/
        boost::shared_ptr<v3d::gl::Program> program();

     private:
        boost::shared_ptr<v3d::gl::Program> program_;
    };
};  // namespace v3d::asset
