/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Material.h"

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

Material::Material(const glm::vec3 & ambient, const glm::vec3 & diffuse, const glm::vec3 & specular, float shininess) :
    ambient_(ambient),
    specular_(specular),
    diffuse_(diffuse),
    shininess_(shininess),
    name_("Material") {
}

void Material::uniform(const std::string & name) {
    name_ = name;
}

void Material::program(boost::shared_ptr<v3d::gl::Program> program) {
    std::string uniform;
    program->enable();
    uniform = name_ + std::string(".Ka");
    unsigned int ambient = program->uniform(uniform);
    uniform = name_ + std::string(".Kd");
    unsigned int diffuse = program->uniform(uniform);
    uniform = name_ + std::string(".Ks");
    unsigned int specular = program->uniform(uniform);
    uniform = name_ + std::string(".Shininess");
    unsigned int shininess = program->uniform(uniform);

    glUniform3fv(ambient, 1, glm::value_ptr(ambient_));
    glUniform3fv(diffuse, 1, glm::value_ptr(diffuse_));
    glUniform3fv(specular, 1, glm::value_ptr(specular_));
    glUniform1f(shininess, shininess_);
}
