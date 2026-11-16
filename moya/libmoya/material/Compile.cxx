/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/material/Compile.h>

#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/program/Displacement.h>
#include <moya/libmoya/kernel/program/Parameters.h>
#include <moya/libmoya/kernel/program/Program.h>
#include <moya/libmoya/material/Fold.h>
#include <moya/libmoya/material/internal/Definitions.h>
#include <moya/libmoya/material/internal/Emitter.h>
#include <moya/libmoya/material/internal/Network.h>
#include <moya/libmoya/material/internal/TerminalInput.h>
#include <moya/libmoya/material/internal/Translate.h>
#include <moya/libmoya/material/Library.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/texture/SearchPath.h>
#include <MaterialXCore/Definition.h>
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Exception.h>
#include <MaterialXCore/Interface.h>
#include <MaterialXCore/Node.h>
#include <MaterialXCore/Types.h>
#include <MaterialXCore/Value.h>
#include <MaterialXFormat/File.h>
#include <MaterialXFormat/XmlIo.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace v3d::moya::material {

namespace {

namespace mx = MaterialX;
using internal::Network;
using internal::TerminalInput;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";
constexpr const char * kDisplacementFloat = "ND_displacement_float";
constexpr const char * kDisplacementVector = "ND_displacement_vector3";

/** Whether a node is a shader, a closure or a material, whose inputs feed moya's BSDF library. **/
bool shading(const mx::NodePtr & node) {
    static const std::set<std::string> kTypes { "surfaceshader", "volumeshader", "displacementshader", "lightshader", "BSDF", "EDF", "VDF", "material" };
    return kTypes.contains(node->getType());
}

/**
 * Whether a node is an RI displacement, of a definition ND_ri_<shader>_displacementshader. Its
 * graph is flattened into the network, so the displacement it ends in is the terminal.
 **/
bool riDisplacement(const mx::NodePtr & node) {
    const std::string definition = node->getNodeDefString();
    const std::string suffix = "_displacementshader";
    return definition.starts_with("ND_ri_") && definition.size() > suffix.size() && definition.ends_with(suffix);
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
 * Puts the definitions of an RI shader's document on the scene's shader search path, the file
 * `<shader>.mtlx` in the first of its directories that holds one, before the network's libraries.
 * A definition of the document's therefore wins over one of the same name in the libraries. A
 * shader search path that is empty names no document, and a document MaterialX cannot read is
 * reported.
 **/
void readShaderDocument(Network & network, const scene::Scene & scene, const std::string & shader) {
    const std::vector<scene::Handle> options = scene.objects("options");
    const std::vector<std::string> directories =
        options.empty() ? std::vector<std::string> {} : texture::directoriesOf(firstString(scene, options.front(), "shaderSearchPath"));
    if (directories.empty()) {
        return;
    }
    const std::optional<std::string> file = texture::found(shader + ".mtlx", directories);
    if (!file) {
        return;
    }
    try {
        const mx::DocumentPtr own = mx::createDocument();
        mx::readFromXmlFile(own, mx::FilePath(*file));
        own->importLibrary(network.document->getDataLibrary());
        network.document->setDataLibrary(own);
    } catch (const mx::Exception & error) {
        network.report("shader " + shader + "'s document " + *file + " is not a valid MaterialX document, so it is not read: " + error.what());
    }
}

/**
 * The definition an RI surface's name finds: ND_ri_<shader>_surfaceshader, from a document of
 * the shader's on the scene's shader search path, or from the libraries. RI's default surface is
 * RI's matte. A name neither defines is reported, and shades as RI's matte.
 **/
std::string riDefinition(Network & network, const scene::Scene & scene, const scene::Handle & handle) {
    const std::string shader = firstString(scene, handle, "shader");
    const std::string matte = "ND_ri_matte_surfaceshader";
    if (shader == "defaultsurface") {
        return matte;
    }
    readShaderDocument(network, scene, shader);
    const std::string definition = "ND_ri_" + shader + "_surfaceshader";
    if (!network.document->getDataLibrary()->getNodeDef(definition)) {
        network.report("shader " + scene.name(handle) + " is RI's " + shader + ", which moya does not define, so it shades as RI's matte");
        return matte;
    }
    return definition;
}

/**
 * The definition an RI displacement's name finds: ND_ri_<shader>_displacementshader, from a
 * document of the shader's on the scene's shader search path, or from the libraries. A name
 * neither defines is reported, and gives no definition, so the material does not displace.
 **/
std::optional<std::string> riDisplacementDefinition(Network & network, const scene::Scene & scene, const scene::Handle & handle) {
    const std::string shader = firstString(scene, handle, "shader");
    readShaderDocument(network, scene, shader);
    const std::string definition = "ND_ri_" + shader + "_displacementshader";
    if (!network.document->getDataLibrary()->getNodeDef(definition)) {
        network.report("shader " + scene.name(handle) + " is RI's displacement " + shader + ", which moya does not define, so it does not displace");
        return std::nullopt;
    }
    return definition;
}

/**
 * Adds the MaterialX node a scene's shader node names, with the inputs its primvars give, or
 * nothing when it names no definition or is neither a MaterialX node nor an RI shader. An RI node
 * is a surface, or a displacement when it is a displacement terminal.
 **/
mx::NodePtr addNode(Network & network, const scene::Scene & scene, const scene::Handle & handle, bool displacing = false) {
    const std::string & name = scene.name(handle);
    const std::string language = firstString(scene, handle, "language");
    if (language != "materialx" && language != "ri") {
        network.report("shader " + name + " is not a MaterialX node, so it is left out of its material");
        return nullptr;
    }
    std::string definition = firstString(scene, handle, "shader");
    if (language == "ri" && displacing) {
        const std::optional<std::string> found = riDisplacementDefinition(network, scene, handle);
        if (!found) {
            return nullptr;
        }
        definition = *found;
    } else if (language == "ri") {
        definition = riDefinition(network, scene, handle);
    }
    const mx::NodeDefPtr def = network.document->getDataLibrary()->getNodeDef(definition);
    if (!def) {
        network.report("shader " + name + " names " + definition + ", which MaterialX does not define, so it is left out");
        return nullptr;
    }
    const mx::NodePtr node = network.document->addNodeInstance(def, network.document->createValidChildName(name));
    // an RI shader that shades as RI's matte was reported once, and its own parameters are its
    const bool standsIn = language == "ri" && !displacing && definition != "ND_ri_" + firstString(scene, handle, "shader") + "_surfaceshader";
    for (const scene::Declaration & declaration : scene.primvars(handle)) {
        const mx::InputPtr declared = def->getActiveInput(declaration.name);
        if (!declared) {
            if (!standsIn) {
                network.report(joined({ "shader ", name, "'s ", declaration.name, " is not an input of ", definition, ", so it is left out" }));
            }
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
 * The network of one of a material's terminals, its surface or its displacement: every node that
 * feeds the terminal's node through the scene's connections, read into a MaterialX document and
 * flattened.
 **/
Network network(const Library & library, const scene::Scene & scene, const scene::Handle & material, const std::string & terminal) {
    Network out;
    out.document = mx::createDocument();
    out.document->setDataLibrary(library.definitions().get());
    const scene::Handle surface = firstHandle(scene, material, terminal);
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
        nodes.emplace(surface.index, addNode(out, scene, surface, terminal == "displacement"));
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
        out.terminal = nodes.at(surface.index);
        if (out.terminal && terminal == "surface") {
            out.terminal = internal::translated(out, out.terminal);
        }
        mx::NodePtr made;
        if (out.terminal) {
            made = out.document->addMaterialNode(out.document->createValidChildName(scene.name(material)), out.terminal);
        }
        const bool flattensTerminal = out.terminal && riDisplacement(out.terminal);
        out.document->flattenSubgraphs(mx::EMPTY_STRING, [](const mx::NodePtr & node) {
            return !shading(node) || riDisplacement(node);
        });
        // an RI displacement's graph took its place, and the displacement the graph ends in is
        // what the material's input reads now
        if (flattensTerminal) {
            const mx::InputPtr reads = made->getInput("displacementshader");
            out.terminal = reads ? reads->getConnectedNode() : nullptr;
            // a node a graph was flattened into names its definition by its category and type alone
            const mx::NodeDefPtr def = out.terminal ? out.terminal->getNodeDef() : nullptr;
            if (def) {
                out.terminal->setNodeDefString(def->getName());
            }
        }
    } catch (const mx::Exception & error) {
        out.report("material " + scene.name(material) + " is not a valid MaterialX network: " + error.what());
        out.terminal = nullptr;
    }
    return out;
}

/** Whether an input of open_pbr_surface sets its emission, which a light gives as a constant. **/
bool emits(const std::string & name) {
    return name.rfind("emission_", 0) == 0;
}

/**
 * Compiles the inputs of a terminal's node into the registers of its parameters. An input a node
 * feeds is compiled from that node while compiling, and otherwise gives the default of the output
 * that feeds it. An emission input a node feeds gives that default too unless emitting.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - whether compiling, then whether emitting
void fill(internal::Emitter & emitter, Network & network, bool compiling, bool emitting, std::span<const TerminalInput> inputs) {
    const mx::NodePtr & surface = network.terminal;
    const mx::NodeDefPtr def = surface->getNodeDef();
    for (const TerminalInput & parameter : inputs) {
        const std::uint32_t width = parameter.width;
        const mx::InputPtr input = surface->getInput(parameter.name);
        const mx::NodePtr from = input ? input->getConnectedNode() : nullptr;
        if (from) {
            const std::string & output = input->getOutputString();
            const bool compiled = compiling && (emitting || !emits(parameter.name));
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
    Network held = network(library, scene, material, "surface");
    Compiled out;
    if (held.terminal) {
        const std::string definition = held.terminal->getNodeDefString();
        if (definition != kOpenPbr) {
            held.report("material " + scene.name(material) + "'s surface is " + definition + ", which moya does not render, so it takes OpenPBR's defaults");
            out.material.layered = 1;
        } else {
            internal::Emitter compiled(held, primvars, kernel::program::kParameterRegisters);
            fill(compiled, held, true, true, internal::kOpenPbrInputs);
            // a light's emission is made before rendering, so emission that varies is a default
            const Folded emitted = fold(compiled.instructions(), compiled.values());
            const bool varies = !compiled.overflowed() &&
                !(emitted.constantFrom(kernel::program::kEmissionLuminance, 1U) && emitted.constantFrom(kernel::program::kEmissionColour, 3U));
            internal::Emitter steady(held, primvars, kernel::program::kParameterRegisters);
            if (varies) {
                held.report("material " + scene.name(material) + "'s emission varies over its surface, so each node feeding it gives its default");
                fill(steady, held, true, false, internal::kOpenPbrInputs);
            }
            const internal::Emitter & chosen = varies ? steady : compiled;
            internal::Emitter constants(held, primvars, kernel::program::kParameterRegisters);
            if (chosen.overflowed()) {
                held.report("material " + scene.name(material) + " needs more registers than a program holds, so each node feeding its surface gives its default");
                fill(constants, held, false, false, internal::kOpenPbrInputs);
            }
            const internal::Emitter & emitter = chosen.overflowed() ? constants : chosen;
            out.instructions = emitter.instructions();
            out.values = emitter.values();
            out.colours = emitter.colours();
            out.spaces = emitter.spaces();
            out.images = emitter.images();
            out.material.program = kernel::program::Program { .instructions = 0,
                .count = static_cast<std::uint32_t>(out.instructions.size()),
                .values = 0,
                .size = static_cast<std::uint32_t>(out.values.size()) };
            const Folded folded = fold(out.instructions, out.values);
            out.material.surface = kernel::program::parameters(folded.values.data());
            const mx::ValuePtr thin = held.terminal->getInputValue("geometry_thin_walled");
            out.material.surface.thinWalled = thin && thin->isA<bool>() && thin->asA<bool>() ? 1U : 0U;
            out.material.programmedOpacity = folded.constantFrom(kernel::program::kGeometryOpacity, 1U) ? 0U : 1U;
            out.material.layered = 1;
        }
    }
    out.reports = std::move(held.reports);
    return out;
}

Compiled compileDisplacement(const Library & library, const scene::Scene & scene, const scene::Handle & material, std::vector<std::string> & primvars) {
    Network held = network(library, scene, material, "displacement");
    Compiled out;
    if (held.terminal) {
        const std::string definition = held.terminal->getNodeDefString();
        const bool vector = definition == kDisplacementVector;
        if (definition != kDisplacementFloat && !vector) {
            held.report("material " + scene.name(material) + "'s displacement is " + definition + ", which moya does not displace by, so it is not displaced");
        } else {
            const std::span<const TerminalInput> inputs = vector ? std::span<const TerminalInput>(internal::kDisplacementVectorInputs)
                                                                 : std::span<const TerminalInput>(internal::kDisplacementFloatInputs);
            internal::Emitter compiled(held, primvars, kernel::program::kDisplacementRegisters);
            fill(compiled, held, true, true, inputs);
            internal::Emitter constants(held, primvars, kernel::program::kDisplacementRegisters);
            if (compiled.overflowed()) {
                held.report("material " + scene.name(material) + " needs more registers than a program holds, so each node feeding its displacement gives its default");
                fill(constants, held, false, false, inputs);
            }
            const internal::Emitter & emitter = compiled.overflowed() ? constants : compiled;
            out.instructions = emitter.instructions();
            out.values = emitter.values();
            out.colours = emitter.colours();
            out.spaces = emitter.spaces();
            out.images = emitter.images();
            out.material.program = kernel::program::Program { .instructions = 0,
                .count = static_cast<std::uint32_t>(out.instructions.size()),
                .values = 0,
                .size = static_cast<std::uint32_t>(out.values.size()) };
        }
    }
    out.reports = std::move(held.reports);
    return out;
}

std::string flattened(const Library & library, const scene::Scene & scene, const scene::Handle & material) {
    return mx::writeToXmlString(network(library, scene, material, "surface").document);
}

};  // namespace v3d::moya::material
