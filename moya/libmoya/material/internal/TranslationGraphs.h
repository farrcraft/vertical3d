/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::moya::material::internal {

/**
 * The MaterialX document of moya's own translations onto OpenPBR Surface, for the surface shaders
 * MaterialX ships no translation of: UsdPreviewSurface, glTF PBR and RI's built-in surfaces, which
 * riSurfaces() also defines. Each is a node definition
 * named as MaterialX names its own, ND_<shader>_to_open_pbr_surface. Its inputs are the shader's,
 * with the shader's defaults, and its outputs are OpenPBR's inputs, each named with `_out` after
 * it. A node graph built from standard nodes implements each.
 *
 * UsdPreviewSurface's normal is in tangent space, and its graph reads it as a normal map. glTF
 * PBR's normals are in world space, and compiling a material passes them to OpenPBR's directly, as
 * it passes Standard Surface's.
 **/
const char * translationGraphs();

};  // namespace v3d::moya::material::internal
