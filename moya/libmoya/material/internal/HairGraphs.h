/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::moya::material::internal {

/**
 * The MaterialX document of moya's graphs for the nodes that give a hair's parameters, which
 * MaterialX implements only in its shader generators: `chiang_hair_roughness`,
 * `chiang_hair_absorption_from_color` and `deon_hair_absorption_from_melanin`. Each graph
 * implements MaterialX's own definition from standard nodes, with the formulas of MaterialX's
 * GLSL implementation, so a material is flattened into nodes a program compiles.
 **/
const char * hairGraphs();

};  // namespace v3d::moya::material::internal
