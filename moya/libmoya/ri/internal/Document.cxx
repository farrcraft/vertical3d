/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/ri/internal/Document.h>

#include <moya/libmoya/material/Documents.h>
#include <moya/libmoya/material/Library.h>
#include <moya/libmoya/ri/internal/DocumentReader.h>
#include <moya/libmoya/ri/internal/Objects.h>
#include <moya/libmoya/scene/Scene.h>
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Interface.h>
#include <MaterialXCore/Node.h>

#include <exception>
#include <optional>
#include <string>

#include <boost/filesystem/path.hpp>

namespace v3d::moya::ri::internal {

namespace mx = MaterialX;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the file, then the material in it
std::optional<Terminals> documentMaterial(scene::Scene & scene, Objects & made, const std::string & file, const std::string & material,
    std::string & reason) {
    mx::DocumentPtr document;
    try {
        document = material::readDocument(material::Library::installed(), file);
    } catch (const std::exception & error) {
        reason = error.what();
        return std::nullopt;
    }
    mx::NodePtr chosen;
    for (const mx::NodePtr & node : document->getMaterialNodes()) {
        if (material.empty() || node->getName() == material) {
            chosen = node;
            break;
        }
    }
    if (!chosen) {
        reason = material.empty() ? "it holds no material" : "it holds no material " + material;
        return std::nullopt;
    }
    const auto shaderOf = [&chosen](const char * input) {
        const mx::InputPtr named = chosen->getInput(input);
        return named ? named->getConnectedNode() : nullptr;
    };
    const mx::NodePtr surface = shaderOf("surfaceshader");
    const mx::NodePtr volume = shaderOf("volumeshader");
    if (!surface && !volume) {
        reason = "its material " + chosen->getName() + " names neither a surface shader nor a volume shader";
        return std::nullopt;
    }
    DocumentReader reader(scene, made, boost::filesystem::path(file).parent_path());
    Terminals out{};
    if (surface) {
        out.surface = reader.nodeOf(surface);
    }
    if (const mx::NodePtr displacer = shaderOf("displacementshader")) {
        out.displacement = reader.nodeOf(displacer);
    }
    if (volume) {
        out.volume = reader.nodeOf(volume);
    }
    return out;
}

};  // namespace v3d::moya::ri::internal
