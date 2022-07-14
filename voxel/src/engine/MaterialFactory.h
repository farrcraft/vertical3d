/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../../v3dlibs/gl/Program.h"

#include <boost/shared_ptr.hpp>
#include <glm/glm.hpp>

class MaterialFactory {
 public:
    explicit MaterialFactory(boost::shared_ptr<v3d::gl::Program> program);

    void create(const std::string & name, const glm::vec3 & color);

 private:
    boost::shared_ptr<v3d::gl::Program> program_;
};
