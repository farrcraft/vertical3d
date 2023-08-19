/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "Asset.h"
#include "../gl/Shader.h"

#include <boost/shared_ptr.hpp>

namespace v3d::asset {
    /**
     **/
    class Shader : public Asset {
    public:
        /**
         **/
        Shader(const std::string& name, Type t, boost::shared_ptr<v3d::gl::Shader> shader);

        /**
         **/
        boost::shared_ptr<v3d::gl::Shader> shader();

    private:
        boost::shared_ptr<v3d::gl::Shader> shader_;
    };
};  // namespace v3d::asset
