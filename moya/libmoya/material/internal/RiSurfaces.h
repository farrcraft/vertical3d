/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

namespace v3d::moya::material::internal {

/**
 * The MaterialX elements that define RI's built-in surfaces and translate each onto OpenPBR
 * Surface, to go in a document's body.
 *
 * Each built-in is a node definition named ND_ri_<shader>_surfaceshader, whose inputs are RI's
 * parameters with RI's defaults, its `Cs` and its `Os`. Its translation is
 * ND_ri_<shader>_to_open_pbr_surface, of the same inputs, which a node graph of standard nodes
 * implements. RI's lobes add without conserving energy and OpenPBR's layers conserve it, so each
 * graph is near the shader, not equal to it:
 *
 * - Kd weighs the base and Ks the specular layer, and `Ka` is taken and not used, because the
 *   integrator gathers every light.
 * - The colour is the geometry's `Cs` primvar, or the node's `Cs` where the geometry has none, as
 *   RI's is. The opacity is the mean of the node's `Os`, since OpenPBR's is one number. It is the
 *   node's alone, so that a surface whose opacity is one is never cut out.
 * - A roughness is RI's Phong exponent of one over it, made a GGX roughness by Walter and others'
 *   equivalence of a Phong exponent n and a Beckmann alpha of sqrt(2 / (n + 2)). OpenPBR's
 *   roughness is the square root of that alpha.
 * - `metal` is a conductor of the colour. `shinymetal` adds a mirror of weight Kr, which OpenPBR
 *   has no second lobe for, so its roughness is the glossy lobe's in proportion to Ks against
 *   Ks and Kr together.
 * - `glass` refracts and reflects sharply, as its traced rays do, with OpenPBR's index of its eta
 *   and its specular layer weighed by Kr. It is opaque, as the shader makes it.
 * - `paintedplastic` multiplies its colour by its texturename's image at s and one less t, since
 *   RI's t runs down an image and MaterialX's v up it.
 * - `constant` emits its node's `Cs` and reflects nothing. Its colour is the node's alone, because
 *   a material's emission has to be the same at every point.
 **/
std::string riSurfaces();

};  // namespace v3d::moya::material::internal
