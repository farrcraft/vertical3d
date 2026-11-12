/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/material/Compile.h>

#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/program/Parameters.h>
#include <moya/libmoya/kernel/program/Program.h>
#include <moya/libmoya/material/internal/Definitions.h>
#include <moya/libmoya/material/internal/Emitter.h>
#include <moya/libmoya/material/internal/Network.h>
#include <moya/libmoya/material/internal/OpenPbrInput.h>
#include <moya/libmoya/material/Library.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <MaterialXCore/Definition.h>
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Exception.h>
#include <MaterialXCore/Interface.h>
#include <MaterialXCore/Node.h>
#include <MaterialXCore/Types.h>
#include <MaterialXCore/Value.h>
#include <MaterialXFormat/XmlIo.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace v3d::moya::material {

namespace {

namespace mx = MaterialX;
using internal::Network;
using internal::OpenPbrInput;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";

/** Whether a node is a shader, a closure or a material, whose inputs feed moya's BSDF library. **/
bool shading(const mx::NodePtr & node) {
    static const std::set<std::string> kTypes { "surfaceshader", "volumeshader", "displacementshader", "lightshader", "BSDF", "EDF", "VDF", "material" };
    return kTypes.contains(node->getType());
}

std::string firstString(const scene::Scene & scene, const scene::Handle & object, const std::string & parameter) {
    return scene.value(object, parameter).stringArray().front();
}

scene::Handle firstHandle(const scene::Scene & scene, const scene::Handle & object, const std::string & parameter) {
    return scene.value(object, parameter).handleArray().front();
}

/** A primvar's value as MaterialX writes a value of a type, or nothing when it cannot be one. **/
std::optional<std::string> valueString(const scene::Value & value, const std::string & type) {
    std::vector<std::string> parts;
    switch (value.type()) {
        case scene::Type::Handle:
            return std::nullopt;
        case scene::Type::Integer:
            for (const std::int32_t element : value.integerArray()) {
                if (type == "boolean") {
                    parts.emplace_back(element != 0 ? "true" : "false");
                } else {
                    parts.push_back(std::to_string(element));
                }
            }
            break;
        case scene::Type::String:
            parts = value.stringArray();
            break;
        default:
            for (const float element : value.floatArray()) {
                parts.push_back(mx::toValueString(element));
            }
            break;
    }
    std::string joined;
    for (const std::string & part : parts) {
        joined += joined.empty() ? part : ", " + part;
    }
    if (!mx::Value::createValueFromStrings(joined, type)) {
        return std::nullopt;
    }
    return joined;
}

/** Pieces of text, one after another. **/
std::string joined(std::initializer_list<std::string_view> pieces) {
    std::string out;
    for (const std::string_view piece : pieces) {
        out += piece;
    }
    return out;
}

/**
 * Adds the MaterialX node a scene's shader node names, with the inputs its primvars give, or
 * nothing when it names no definition or is not a MaterialX node.
 **/
mx::NodePtr addNode(Network & network, const scene::Scene & scene, const scene::Handle & handle) {
    const std::string & name = scene.name(handle);
    if (firstString(scene, handle, "language") != "materialx") {
        network.report("shader " + name + " is not a MaterialX node, so it is left out of its material");
        return nullptr;
    }
    const std::string definition = firstString(scene, handle, "shader");
    const mx::NodeDefPtr def = network.document->getDataLibrary()->getNodeDef(definition);
    if (!def) {
        network.report("shader " + name + " names " + definition + ", which MaterialX does not define, so it is left out");
        return nullptr;
    }
    const mx::NodePtr node = network.document->addNodeInstance(def, network.document->createValidChildName(name));
    for (const scene::Declaration & declaration : scene.primvars(handle)) {
        const mx::InputPtr declared = def->getActiveInput(declaration.name);
        if (!declared) {
            network.report(joined({ "shader ", name, "'s ", declaration.name, " is not an input of ", definition, ", so it is left out" }));
            continue;
        }
        const scene::Value & value = scene.value(handle, declaration.name);
        const std::optional<std::string> written = valueString(value, declared->getType());
        if (!written) {
            network.report(joined({ "shader ", name, "'s ", declaration.name, " is not a ", declared->getType(), ", so it is left out" }));
            continue;
        }
        const mx::InputPtr input = node->setInputValue(declaration.name, *written, declared->getType());
        if (!value.space().empty()) {
            input->setColorSpace(value.space());
        }
    }
    return node;
}

/**
 * Connects an input of one node to an output of another, as a scene's connection says, when both
 * nodes were read.
 **/
void connect(Network & network, const scene::Scene & scene, const scene::Handle & connection, const std::map<std::uint32_t, mx::NodePtr> & nodes) {
    const mx::NodePtr & from = nodes.at(firstHandle(scene, connection, "from").index);
    const mx::NodePtr & to = nodes.at(firstHandle(scene, connection, "to").index);
    if (!from || !to) {
        return;
    }
    const std::string & name = scene.name(connection);
    const std::string inputName = firstString(scene, connection, "input");
    const std::string outputName = firstString(scene, connection, "output");
    const mx::NodeDefPtr toDef = to->getNodeDef();
    const mx::NodeDefPtr fromDef = from->getNodeDef();
    const mx::InputPtr declared = toDef->getActiveInput(inputName);
    const mx::OutputPtr output = fromDef->getActiveOutput(outputName);
    if (!declared || !output) {
        network.report("connection " + name + " names an input or an output its nodes do not have, so it is left out");
        return;
    }
    if (declared->getType() != output->getType()) {
        network.report("connection " + name + " feeds a " + output->getType() + " to a " + declared->getType() + ", so it is left out");
        return;
    }
    const mx::InputPtr input = to->getInput(inputName) ? to->getInput(inputName) : to->addInputFromNodeDef(inputName);
    input->removeAttribute(mx::ValueElement::VALUE_ATTRIBUTE);
    input->setConnectedNode(from);
    if (fromDef->getActiveOutputs().size() > 1U) {
        input->setOutputString(outputName);
    }
}

/**
 * The network of a material's surface terminal: every node that feeds the surface node through
 * the scene's connections, read into a MaterialX document and flattened.
 **/
Network network(const Library & library, const scene::Scene & scene, const scene::Handle & material) {
    Network out;
    out.document = mx::createDocument();
    out.document->setDataLibrary(library.definitions().get());
    const scene::Handle surface = firstHandle(scene, material, "surface");
    if (!surface.valid() || !scene.contains(surface)) {
        return out;
    }
    // every connection, by the scene's index of the node it feeds
    std::multimap<std::uint32_t, scene::Handle> feeding;
    for (const scene::Handle & connection : scene.objects("connection")) {
        const scene::Handle to = firstHandle(scene, connection, "to");
        const scene::Handle from = firstHandle(scene, connection, "from");
        if (to.valid() && from.valid() && scene.contains(to) && scene.contains(from)) {
            feeding.emplace(to.index, connection);
        }
    }
    try {
        // the surface node, then each node that feeds a node already read, each once
        std::map<std::uint32_t, mx::NodePtr> nodes;
        std::vector<scene::Handle> reached { surface };
        nodes.emplace(surface.index, addNode(out, scene, surface));
        for (std::size_t i = 0; i < reached.size(); ++i) {
            const auto [first, last] = feeding.equal_range(reached[i].index);
            for (auto at = first; at != last; ++at) {
                const scene::Handle from = firstHandle(scene, at->second, "from");
                if (!nodes.contains(from.index)) {
                    reached.push_back(from);
                    nodes.emplace(from.index, addNode(out, scene, from));
                }
            }
        }
        for (const scene::Handle & to : reached) {
            const auto [first, last] = feeding.equal_range(to.index);
            for (auto at = first; at != last; ++at) {
                connect(out, scene, at->second, nodes);
            }
        }
        out.surface = nodes.at(surface.index);
        if (out.surface) {
            out.document->addMaterialNode(out.document->createValidChildName(scene.name(material)), out.surface);
        }
        out.document->flattenSubgraphs(mx::EMPTY_STRING, [](const mx::NodePtr & node) {
            return !shading(node);
        });
    } catch (const mx::Exception & error) {
        out.report("material " + scene.name(material) + " is not a valid MaterialX network: " + error.what());
        out.surface = nullptr;
    }
    return out;
}

/** Whether an input of open_pbr_surface sets its emission, which a light gives as a constant. **/
bool emits(const std::string & name) {
    return name.rfind("emission_", 0) == 0;
}

/**
 * Compiles the inputs of a surface of open_pbr_surface into the registers of its parameters. An
 * input a node feeds is compiled from that node while compiling, and otherwise gives the default
 * of the output that feeds it. An emission input a node feeds always gives that default.
 **/
void fill(internal::Emitter & emitter, Network & network, bool compiling) {
    const mx::NodePtr & surface = network.surface;
    const mx::NodeDefPtr def = surface->getNodeDef();
    for (const OpenPbrInput & parameter : internal::kOpenPbrInputs) {
        const std::uint32_t width = parameter.colour ? 3U : 1U;
        const mx::InputPtr input = surface->getInput(parameter.name);
        const mx::NodePtr from = input ? input->getConnectedNode() : nullptr;
        if (from) {
            const std::string & output = input->getOutputString();
            const bool compiled = compiling && !emits(parameter.name);
            emitter.copyValue(compiled ? emitter.output(from, output) : emitter.uncompiled(from, output), parameter.at, width);
            continue;
        }
        const mx::InputPtr declared = def->getActiveInput(parameter.name);
        const std::string space = (input ? input : declared)->getActiveColorSpace();
        emitter.constant(parameter.at, surface->getInputValue(parameter.name), declared->getType(), space);
    }
}

};  // namespace

Compiled compile(const Library & library, const scene::Scene & scene, const scene::Handle & material, std::vector<std::string> & primvars) {
    Network held = network(library, scene, material);
    Compiled out;
    if (held.surface) {
        const std::string definition = held.surface->getNodeDefString();
        if (definition != kOpenPbr) {
            held.report("material " + scene.name(material) + "'s surface is " + definition + ", which moya does not render, so it is diffuse");
        } else {
            internal::Emitter compiled(held, primvars);
            fill(compiled, held, true);
            internal::Emitter constants(held, primvars);
            if (compiled.overflowed()) {
                held.report("material " + scene.name(material) + " needs more registers than a program holds, so each node feeding its surface gives its default");
                fill(constants, held, false);
            }
            const internal::Emitter & emitter = compiled.overflowed() ? constants : compiled;
            out.instructions = emitter.instructions();
            out.values = emitter.values();
            out.colours = emitter.colours();
            out.spaces = emitter.spaces();
            out.material.program = kernel::program::Program { .instructions = 0,
                .count = static_cast<std::uint32_t>(out.instructions.size()),
                .values = 0,
                .size = static_cast<std::uint32_t>(out.values.size()) };
            out.material.surface = kernel::program::parameters(out.values.data());
            const mx::ValuePtr thin = held.surface->getInputValue("geometry_thin_walled");
            out.material.surface.thinWalled = thin && thin->isA<bool>() && thin->asA<bool>() ? 1U : 0U;
            out.material.layered = 1;
        }
    }
    out.reports = std::move(held.reports);
    return out;
}

std::string flattened(const Library & library, const scene::Scene & scene, const scene::Handle & material) {
    return mx::writeToXmlString(network(library, scene, material).document);
}

};  // namespace v3d::moya::material
