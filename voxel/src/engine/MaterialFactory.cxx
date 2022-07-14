/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "MaterialFactory.h"

#include <string>

#include "Material.h"

MaterialFactory::MaterialFactory(boost::shared_ptr<v3d::gl::Program> program) :
    program_(program) {
}

void MaterialFactory::create(const std::string & name, const glm::vec3 & color) {
    Material material(
        color,  // ambient
        color,  // diffuse
        glm::vec3(0.8f, 0.8f, 0.8f),  // specular
        100.0f);  // shininess

    material.uniform(name);
    material.program(program_);
}
