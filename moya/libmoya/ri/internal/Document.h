/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/ri/internal/Objects.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <optional>
#include <string>

namespace v3d::moya::ri::internal {

/** A document material's terminal nodes: its surface, and its displacement if it has one. **/
struct Terminals {
    scene::Handle surface;
    std::optional<scene::Handle> displacement;
};

/**
 * The network of a material in a MaterialX document, made in the scene as shader nodes of
 * language `materialx` and the connections between them, and its terminal nodes.
 *
 * The material is the material node of the name given, or the document's first material node
 * when the name is empty. Its network is the node its `surfaceshader` input names, the node its
 * `displacementshader` input names when it has one, and every node either reads, through node
 * graphs and their interface inputs. Each becomes a shader node of the definition MaterialX's
 * standard libraries resolve it to, with each input that holds a value as a constant of that
 * value. A colour keeps the space the document names for it. A filename that is relative is taken
 * from the document's directory. Each connection between two nodes becomes a connection object,
 * naming the output it reads when the input names one.
 *
 * @return the terminal nodes, or nothing when the document cannot be read, the libraries are not
 *         installed, or the document holds no such material, with why in reason
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the file, then the material in it
std::optional<Terminals> documentMaterial(scene::Scene & scene, Objects & made, const std::string & file, const std::string & material,
    std::string & reason);

};  // namespace v3d::moya::ri::internal
