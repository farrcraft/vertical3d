/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::uniform {

/**
 * Points spread evenly over a shape, each from two numbers in [0, 1). A stratified pair of
 * numbers gives stratified points, because each map is continuous.
 **/

/**
 * Shirley's concentric map from the unit square onto the unit disc, which keeps a stratified
 * square stratified on the disc.
 **/
glm::vec2 disc(const glm::vec2 & square);

/** A point on the triangle with these corners, spread evenly by area. **/
glm::vec3 triangle(const glm::vec3 & first, const glm::vec3 & second, const glm::vec3 & third, const glm::vec2 & square);

/** A point on the unit sphere, spread evenly by area. **/
glm::vec3 sphere(const glm::vec2 & square);

};  // namespace v3d::render::offline::uniform
