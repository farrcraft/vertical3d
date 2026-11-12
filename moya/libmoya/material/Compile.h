/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/scene/Handle.h>

#include <string>
#include <vector>

namespace v3d::moya::scene {
class Scene;
};  // namespace v3d::moya::scene

namespace v3d::moya::material {

class Library;

/** A material compiled for shading, and what compiling it left out and why. **/
struct Compiled {
    kernel::surface::Material material;
    std::vector<std::string> reports;
};

/**
 * Compiles a scene's material whose surface terminal is a node of language `materialx`.
 *
 * The network is the surface node and every node that feeds it through the scene's connections.
 * Each node becomes the MaterialX node its `shader` names, a node definition such as
 * ND_open_pbr_surface_surfaceshader, with the inputs its primvars give. A node that names no
 * definition the library holds, or is written in another language, is left out, and each input it
 * feeds keeps its own value. Every node that a node graph implements is then flattened into the
 * nodes of the graph, as MaterialX flattens it. Shader, closure and material nodes are not
 * flattened, because their inputs feed moya's BSDF library.
 *
 * A surface of `open_pbr_surface` makes the material layered, by OpenPBR's parameters. Each input
 * that is not connected gives the value its node holds, or its definition's default. Each input
 * that is connected gives the default of the output that feeds it, and the node that feeds it is
 * reported. A surface of any other definition is reported, and leaves the material diffuse.
 *
 * A colour input is taken as ACEScg, whatever space its primvar names.
 **/
Compiled compile(const Library & library, const scene::Scene & scene, const scene::Handle & material);

/** The flattened MaterialX document that compile() reads a material's surface from, as XML. **/
std::string flattened(const Library & library, const scene::Scene & scene, const scene::Handle & material);

};  // namespace v3d::moya::material
