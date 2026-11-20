/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/ri/internal/DocumentReader.h>

#include <moya/libmoya/ri/internal/Objects.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <MaterialXCore/Definition.h>
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Interface.h>
#include <MaterialXCore/Node.h>

#include <cstdint>
#include <exception>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem/path.hpp>

#include <glm/vec3.hpp>

namespace v3d::moya::ri::internal {

namespace {

namespace mx = MaterialX;

/** The comma-separated numbers of a MaterialX value, or none when one is not a number. **/
std::vector<float> numbersOf(const std::string & text) {
    std::vector<float> out;
    std::stringstream stream(text);
    std::string part;
    while (std::getline(stream, part, ',')) {
        try {
            out.push_back(std::stof(part));
        } catch (const std::exception &) {
            return {};
        }
    }
    return out;
}

};  // namespace

DocumentReader::DocumentReader(scene::Scene & scene, Objects & made, boost::filesystem::path directory) :
    scene_(scene), made_(made), directory_(std::move(directory)) {
}

scene::Handle DocumentReader::nodeOf(const mx::NodePtr & node) {
    const auto found = nodes_.find(node->getNamePath());
    if (found != nodes_.end()) {
        return found->second;
    }
    const scene::Handle out = made_.make("shader", made_.unique("shader", node->getName()));
    nodes_.emplace(node->getNamePath(), out);
    const mx::NodeDefPtr def = node->getNodeDef();
    scene_.set(out, "shader", scene::Value::string(def ? def->getName() : node->getCategory()));
    scene_.set(out, "language", scene::Value::string("materialx"));
    for (const mx::InputPtr & input : node->getInputs()) {
        const std::optional<Upstream> from = upstream(input);
        if (from) {
            connect(nodeOf(from->node), out, from->node->getName() + " to " + input->getName() + " of " + node->getName(), input->getName(), from->output);
            continue;
        }
        const mx::InputPtr interface = input->hasInterfaceName() ? input->getInterfaceInput() : nullptr;
        const std::optional<scene::Value> value = valueOf(interface ? interface : input, input->getType());
        if (value) {
            scene_.declare(out, scene::Declaration { .name = input->getName(), .type = value->type(), .count = static_cast<std::uint32_t>(value->size()), .fallback = *value });
            scene_.set(out, input->getName(), *value);
        }
    }
    return out;
}

std::optional<DocumentReader::Upstream> DocumentReader::upstream(const mx::InputPtr & input) {
    if (input->hasInterfaceName()) {
        const mx::InputPtr outer = input->getInterfaceInput();
        return outer && outer != input ? upstream(outer) : std::nullopt;
    }
    if (input->hasNodeGraphString()) {
        const mx::NodeGraphPtr graph = input->getDocument()->getNodeGraph(input->getNodeGraphString());
        if (!graph || graph->getOutputs().empty()) {
            return std::nullopt;
        }
        const mx::OutputPtr output = input->hasOutputString() ? graph->getOutput(input->getOutputString()) : graph->getOutputs().front();
        const mx::NodePtr node = output ? output->getConnectedNode() : nullptr;
        if (!node) {
            return std::nullopt;
        }
        return Upstream { .node = node, .output = output->hasOutputString() ? output->getOutputString() : std::string() };
    }
    const mx::NodePtr node = input->getConnectedNode();
    if (!node) {
        return std::nullopt;
    }
    return Upstream { .node = node, .output = input->hasOutputString() ? input->getOutputString() : std::string() };
}

std::optional<scene::Value> DocumentReader::valueOf(const mx::InputPtr & input, const std::string & type) const {
    if (!input->hasValueString()) {
        return std::nullopt;
    }
    const std::string & text = input->getValueString();
    if (type == "string") {
        return scene::Value::string(text);
    }
    if (type == "filename") {
        const boost::filesystem::path path(text);
        return scene::Value::string(text.empty() || path.is_absolute() ? text : (directory_ / path).generic_string());
    }
    if (type == "boolean") {
        return scene::Value::integer(text == "true" ? 1 : 0);
    }
    const std::vector<float> numbers = numbersOf(text);
    if (numbers.empty()) {
        return std::nullopt;
    }
    if (type == "integer") {
        return scene::Value::integer(static_cast<std::int32_t>(numbers.front()));
    }
    if (type == "float") {
        return scene::Value::number(numbers.front());
    }
    if (type == "color3" && numbers.size() == 3U) {
        return scene::Value::colour(glm::vec3 { numbers[0], numbers[1], numbers[2] }, input->getActiveColorSpace());
    }
    return scene::Value::floats(numbers);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the nodes, then the input and output
void DocumentReader::connect(const scene::Handle & from, const scene::Handle & to, const std::string & name, const std::string & input, const std::string & output) {
    const scene::Handle out = made_.make("connection", made_.unique("connection", name));
    scene_.set(out, "from", scene::Value::handle(from));
    scene_.set(out, "to", scene::Value::handle(to));
    scene_.set(out, "input", scene::Value::string(input));
    if (!output.empty()) {
        scene_.set(out, "output", scene::Value::string(output));
    }
}

};  // namespace v3d::moya::ri::internal
