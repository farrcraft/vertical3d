/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec3.hpp>

namespace v3d::render::offline::sl::runtime {

/**
 * The values a hider has for one shading point, which a surface shader reads as its globals.
 * Every hider fills in the same fields, so a shader sees the same thing under either.
 **/
class Point final {
 public:
    glm::vec3 position = glm::vec3(0.0F);   /**< P **/
    glm::vec3 normal = glm::vec3(0.0F);     /**< N **/
    glm::vec3 geometric = glm::vec3(0.0F);  /**< Ng **/
    glm::vec3 incident = glm::vec3(0.0F);   /**< I **/
    glm::vec3 colour = glm::vec3(1.0F);     /**< Cs **/
    glm::vec3 opacity = glm::vec3(1.0F);    /**< Os, and Oi until the shader sets it **/
    float s = 0.0F;
    float t = 0.0F;
    float u = 0.0F;
    float v = 0.0F;
    float du = 0.0F;                        /**< zero for a hider with no neighbouring point to difference **/
    float dv = 0.0F;
};

};  // namespace v3d::render::offline::sl::runtime
