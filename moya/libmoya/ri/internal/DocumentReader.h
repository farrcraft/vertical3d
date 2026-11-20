/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/ri/internal/Objects.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <MaterialXCore/Interface.h>
#include <MaterialXCore/Node.h>

#include <map>
#include <optional>
#include <string>

#include <boost/filesystem/path.hpp>

namespace v3d::moya::ri::internal {

/**
 * Makes the nodes of a MaterialX document in a scene, as documentMaterial() says, each once.
 * Relative files are taken from the document's directory.
 **/
class DocumentReader final {
 public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the scene, then what it makes
    DocumentReader(scene::Scene & scene, Objects & made, boost::filesystem::path directory);

    /** The scene's shader node of a document's node, made with the nodes it reads. **/
    scene::Handle nodeOf(const MaterialX::NodePtr & node);

 private:
    /** The node an input reads, and its output, which is empty for a node's only output. **/
    struct Upstream {
        MaterialX::NodePtr node;
        std::string output;
    };

    /**
     * The node an input reads: the node it names, the node a node graph's output it names reads,
     * or what the interface input of its graph reads. Nothing for an input that holds a value.
     **/
    static std::optional<Upstream> upstream(const MaterialX::InputPtr & input);

    /** An input's value as the scene holds one of its MaterialX type, or nothing without one. **/
    std::optional<scene::Value> valueOf(const MaterialX::InputPtr & input, const std::string & type) const;

    /** A connection from one node's output to another's input, named after the nodes. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the nodes, then the input and output
    void connect(const scene::Handle & from, const scene::Handle & to, const std::string & name, const std::string & input, const std::string & output);

    scene::Scene & scene_;
    Objects & made_;
    boost::filesystem::path directory_;
    std::map<std::string, scene::Handle> nodes_;
};

};  // namespace v3d::moya::ri::internal
