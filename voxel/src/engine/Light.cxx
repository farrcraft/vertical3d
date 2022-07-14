/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Light.h"

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

Light::Light(const glm::vec4 & position, const glm::vec3 & ambient, const glm::vec3 & diffuse, const glm::vec3 & specular) :
    position_(position),
    ambient_(ambient),
    diffuse_(diffuse),
    specular_(specular) {
}

void Light::program(boost::shared_ptr<v3d::gl::Program> program) {
    program->enable();
    unsigned int position = program->uniform("Light.Position");
    unsigned int ambient = program->uniform("Light.La");
    unsigned int diffuse = program->uniform("Light.Ld");
    unsigned int specular = program->uniform("Light.Ls");

    glUniform4fv(position, 1, glm::value_ptr(position_));
    glUniform3fv(ambient, 1, glm::value_ptr(ambient_));
    glUniform3fv(diffuse, 1, glm::value_ptr(diffuse_));
    glUniform3fv(specular, 1, glm::value_ptr(specular_));
}
