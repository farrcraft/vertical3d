/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/ri/internal/Network.h>

#include <moya/libmoya/material/Library.h>
#include <moya/libmoya/ri/internal/Document.h>
#include <moya/libmoya/ri/internal/Primvars.h>
#include <moya/libmoya/scene/Value.h>

#include <cstddef>
#include <exception>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace v3d::moya::ri::internal {

namespace {

/**
 * The type of a node definition's output, or empty when the definition is not defined or the
 * libraries are not installed.
 **/
std::string outputType(const std::string & definition) {
    try {
        return material::Library::installed().type(definition);
    } catch (const std::exception &) {
        return {};
    }
}

/** Whether a Bxdf's node gives the medium a material fills its geometry with, not its surface. **/
bool fills(const std::string & definition) {
    const std::string type = outputType(definition);
    return type == "volumeshader" || type == "VDF";
}

};  // namespace

Network::Network(scene::Scene * scene, Objects * made, Warnings * warnings, const std::vector<Node> & nodes) :
    scene_(scene), made_(made), warnings_(warnings), nodes_(nodes) {
}

std::optional<Terminals> Network::terminals(const std::string & surface) {
    std::optional<std::size_t> surfaceAt;
    std::optional<std::size_t> volumeAt;
    for (std::size_t place = 0; place < nodes_.size(); ++place) {
        if (nodes_[place].bxdf) {
            (fills(nodes_[place].request.type) ? volumeAt : surfaceAt) = place;
        }
    }
    Terminals out{};
    if (surfaceAt && nodes_[*surfaceAt].request.type == "materialx") {
        const Node & document = nodes_[*surfaceAt];
        const std::optional<scene::Value> file = valueOf(document.parameters, "file");
        const std::optional<scene::Value> named = valueOf(document.parameters, "material");
        const std::string path = file && !file->stringArray().empty() ? file->stringArray().front() : std::string();
        const std::string chosen = named && !named->stringArray().empty() ? named->stringArray().front() : std::string();
        std::string reason;
        const std::optional<Terminals> read = documentMaterial(*scene_, *made_, path, chosen, reason);
        if (!read) {
            warnings_->once("document " + path, "the MaterialX document \"" + path + "\" gives no material, because " + reason + ", so the surface is " + surface);
            return std::nullopt;
        }
        out = *read;
        // the Bxdf's other parameters override the inputs of the document's surface, or of its
        // volume when it has no surface
        const std::optional<scene::Handle> overridden = out.surface ? out.surface : out.volume;
        inputsOf(*overridden, document.request.handle, document, *surfaceAt, { "file", "material" });
    } else if (surfaceAt) {
        out.surface = terminal(*surfaceAt);
    }
    // a volume's Bxdf gives the material its medium, over a document's
    if (volumeAt) {
        out.volume = terminal(*volumeAt);
    }
    return out;
}

scene::Handle Network::terminal(std::size_t place) {
    const scene::Handle closure = node(place);
    const std::string type = outputType(nodes_[place].request.type);
    if (type != "BSDF" && type != "VDF") {
        return closure;
    }
    // a closure is a terminal through MaterialX's surface or volume node, which reads it
    const bool bsdf = type == "BSDF";
    const std::string & handle = nodes_[place].request.handle;
    const std::string base = handle.empty() ? nodes_[place].request.type : handle;
    const scene::Handle shader = made_->make("shader", made_->unique("shader", base + (bsdf ? " surface" : " volume")));
    scene_->set(shader, "shader", scene::Value::string(bsdf ? "ND_surface" : "ND_volume"));
    scene_->set(shader, "language", scene::Value::string("materialx"));
    const std::string input = bsdf ? "bsdf" : "vdf";
    const scene::Handle connection = made_->make("connection", made_->unique("connection", base + " to " + input));
    scene_->set(connection, "from", scene::Value::handle(closure));
    scene_->set(connection, "to", scene::Value::handle(shader));
    scene_->set(connection, "input", scene::Value::string(input));
    return shader;
}

scene::Handle Network::node(std::size_t place) {
    const auto found = nodesMade_.find(place);
    if (found != nodesMade_.end()) {
        return found->second;
    }
    const Node & request = nodes_[place];
    const std::string name = made_->unique("shader", request.request.handle.empty() ? request.request.type : request.request.handle);
    const scene::Handle out = made_->make("shader", name);
    nodesMade_.emplace(place, out);
    scene_->set(out, "shader", scene::Value::string(request.request.type));
    scene_->set(out, "language", scene::Value::string("materialx"));
    inputsOf(out, name, request, place, {});
    return out;
}

void Network::inputsOf(const scene::Handle & made, const std::string & name, const Node & request, std::size_t place,
    const std::set<std::string> & except) {
    std::set<std::string> constants = except;
    for (const std::string & input : request.parameters.names()) {
        if (!request.parameters.declaration(input)->reference()) {
            continue;
        }
        constants.insert(input);
        const std::vector<std::string> & values = request.parameters.strings(input);
        const std::string value = values.empty() ? std::string() : values.front();
        const std::size_t colon = value.find(':');
        const std::string handle = value.substr(0, colon);
        const std::string output = colon == std::string::npos ? std::string() : value.substr(colon + 1);
        // a reference reads a node that came before it
        std::size_t from = place;
        for (std::size_t earlier = place; earlier > 0; --earlier) {
            if (nodes_[earlier - 1].request.handle == handle) {
                from = earlier - 1;
                break;
            }
        }
        if (from == place) {
            std::string message = "a reference names ";
            message.append(handle).append(", which no Pattern or Bxdf before it has as its handle, so ").append(input).append(" keeps its value");
            warnings_->once("reference " + handle, message);
            continue;
        }
        const scene::Handle source = node(from);
        std::string named = handle;
        named.append(" to ").append(input).append(" of ").append(name);
        const scene::Handle connection = made_->make("connection", made_->unique("connection", named));
        scene_->set(connection, "from", scene::Value::handle(source));
        scene_->set(connection, "to", scene::Value::handle(made));
        scene_->set(connection, "input", scene::Value::string(input));
        if (!output.empty()) {
            scene_->set(connection, "output", scene::Value::string(output));
        }
    }
    declare(scene_, made, request.parameters, constants, true);
}

};  // namespace v3d::moya::ri::internal
