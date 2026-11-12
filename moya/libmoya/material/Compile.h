/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/scene/Handle.h>

#include <cstdint>
#include <string>
#include <vector>

namespace v3d::moya::scene {
class Scene;
};  // namespace v3d::moya::scene

namespace v3d::moya::material {

class Library;

/**
 * A material compiled for shading, and what compiling it left out and why. A layered material's
 * program runs the instructions here from the values here, and names the first of each as zero.
 * The registers of each colour constant are named with the space the colour is in, and every
 * other value is as the program reads it.
 **/
struct Compiled {
    kernel::surface::Material material;
    std::vector<kernel::program::Instruction> instructions;
    std::vector<float> values;
    std::vector<std::uint32_t> colours;
    std::vector<std::string> spaces;
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
 * A surface of `open_pbr_surface` makes the material layered, and its program gives OpenPBR's
 * parameters. Each input that is not connected gives the value its node holds, or its
 * definition's default, as a constant. Each input that is connected is compiled from the node
 * that feeds it, and from every node that one reads, each once. A node the program does not
 * compile gives the default of its output, and is reported. An emission input that is connected
 * gives that default too, because a material's light is made before rendering. A program that
 * needs more registers than a shading point holds is reported, and each connected input then
 * gives its default. The surface holds the parameters before the program runs.
 *
 * A surface of any other definition is reported, and leaves the material diffuse.
 *
 * A colour constant is in the space its primvar names, or the space its document names for it.
 *
 * A node that reads a primvar names it by its number in primvars, which holds the name of each
 * primvar by its number. A name read first is added at the end. texcoord reads `st`, and
 * geomcolor reads `Cs`.
 **/
Compiled compile(const Library & library, const scene::Scene & scene, const scene::Handle & material, std::vector<std::string> & primvars);

/** The flattened MaterialX document that compile() reads a material's surface from, as XML. **/
std::string flattened(const Library & library, const scene::Scene & scene, const scene::Handle & material);

};  // namespace v3d::moya::material
