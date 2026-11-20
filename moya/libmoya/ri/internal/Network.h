/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/rib/Parameters.h>
#include <api/rib/requests/ShadingNode.h>
#include <moya/libmoya/ri/internal/Document.h>
#include <moya/libmoya/ri/internal/Objects.h>
#include <moya/libmoya/ri/internal/Warnings.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace v3d::moya::ri::internal {

/**
 * Makes the material of RenderMan's shading network in the scene: a shader node of language
 * `materialx` for each node a terminal reads, and a connection for each reference.
 *
 * **Terminals.** The last `Bxdf` of a surface is the surface, and the last `Bxdf` of a volume
 * the volume, the medium inside the geometry the material is given to. A `Bxdf` is a volume's
 * when its definition's output is a `volumeshader` or a `VDF`. A `Bxdf` whose output is a `BSDF`,
 * such as a hair's `ND_chiang_hair_bsdf`, is read through MaterialX's `surface` node, and one
 * whose output is a `VDF` through its `volume` node. A network of no surface's `Bxdf` gives no
 * surface.
 *
 * **Documents.** A `Bxdf` of type `materialx` reads the material its `material` names, or the
 * first, from the MaterialX document its `file` names, with the document's volume unless a
 * volume's `Bxdf` gives one. Its other parameters override the inputs of the document's surface
 * node, or of its volume node when it has no surface.
 *
 * **References.** A `reference` parameter connects its input to the output of the node of the
 * handle its value names, written `handle:output`, or `handle` alone for a node's only output.
 * It reads a node before it, and one that names no such node is logged. Each other parameter is
 * a constant input.
 **/
class Network final {
 public:
    /** A Pattern or a Bxdf request: its node, its parameters, and whether it is a Bxdf. **/
    struct Node {
        v3d::rib::requests::ShadingNode request{};
        v3d::rib::ParameterList parameters{};
        bool bxdf { false };
    };

    /** @param scene, made and warnings outlive this, and the nodes are in the order they came **/
    Network(scene::Scene * scene, Objects * made, Warnings * warnings, const std::vector<Node> & nodes);

    /**
     * The terminal nodes of the material, with every node they read, or nothing when a document
     * it names cannot be read. Only a document gives a displacement.
     *
     * @param surface the name of the surface used in place of a document that cannot be read
     **/
    std::optional<Terminals> terminals(const std::string & surface);

 private:
    /**
     * The terminal node of the Bxdf at a place: its node, or MaterialX's surface or volume node
     * reading it when its output is a BSDF or a VDF.
     **/
    scene::Handle terminal(std::size_t place);

    /** The shader node of the node at a place, made with the nodes it reads, each once. **/
    scene::Handle node(std::size_t place);

    /**
     * Sets a node's inputs from a request's parameters but those named, and connects each
     * reference to the node of the handle it names, among the nodes before a place.
     **/
    void inputsOf(const scene::Handle & made, const std::string & name, const Node & request, std::size_t place,
        const std::set<std::string> & except);

    scene::Scene * scene_;
    Objects * made_;
    Warnings * warnings_;
    const std::vector<Node> & nodes_;
    /** The shader node made for each node, by its place. **/
    std::map<std::size_t, scene::Handle> nodesMade_;
};

};  // namespace v3d::moya::ri::internal
