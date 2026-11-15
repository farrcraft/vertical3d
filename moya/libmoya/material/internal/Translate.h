/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/material/internal/Network.h>
#include <MaterialXCore/Node.h>

namespace v3d::moya::material::internal {

/**
 * A surface node of Standard Surface, UsdPreviewSurface, glTF PBR or an RI surface translated onto
 * OpenPBR Surface, or the node as it was for any other definition. An RI surface's definition is
 * ND_ri_<shader>_surfaceshader, and its translation ND_ri_<shader>_to_open_pbr_surface.
 *
 * The node is replaced by two in the network's document: a translation node, whose definition
 * is ND_<shader>_to_open_pbr_surface, and an open_pbr_surface node that each of its outputs
 * feeds. MaterialX ships Standard Surface's translation, and material::Library reads moya's own
 * for the others. The translation node takes every input of the surface node's definition,
 * with the value or the connection the node gives it, or its definition's default. An input the
 * translation does not take is left out.
 *
 * A normal, a tangent and a coat normal in world space pass to OpenPBR's geometry inputs
 * directly when a node feeds them, and Standard Surface's thin_walled passes as it is. A
 * translation missing from the libraries is reported, and the node is left as it was.
 **/
MaterialX::NodePtr translated(Network & network, const MaterialX::NodePtr & surface);

};  // namespace v3d::moya::material::internal
