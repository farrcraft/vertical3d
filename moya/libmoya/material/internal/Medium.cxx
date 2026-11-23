/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/material/internal/Medium.h>

#include <moya/libmoya/material/internal/MediumRewrite.h>
#include <moya/libmoya/material/internal/Network.h>
#include <moya/libmoya/material/internal/TerminalInput.h>
#include <MaterialXCore/Definition.h>
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Interface.h>
#include <MaterialXCore/Node.h>

#include <optional>
#include <string>

namespace v3d::moya::material::internal {

namespace {

namespace mx = MaterialX;

using Quantity = MediumRewrite::Quantity;
using Source = MediumRewrite::Source;

constexpr const char * kMediumDefinition = "ND_moya_medium";

/** The MaterialX type of an input of a medium's node. **/
const char * typeOf(const TerminalInput & input) {
    if (input.width != 3U) {
        return "float";
    }
    return std::string(input.name) == "emission" ? "color3" : "vector3";
}

/** The definition of the node a volume's shader is rewritten into, made once in a document. **/
mx::NodeDefPtr mediumDefinition(const mx::DocumentPtr & document) {
    if (const mx::NodeDefPtr held = document->getNodeDef(kMediumDefinition)) {
        return held;
    }
    const mx::NodeDefPtr def = document->addNodeDef(kMediumDefinition, "volumeshader", "moya_medium");
    for (const TerminalInput & each : kMediumInputs) {
        def->addInput(each.name, typeOf(each))->setValueString(each.width == 3U ? "0, 0, 0" : "0");
    }
    return def;
}

};  // namespace

mx::NodePtr rewrittenMedium(Network & network) {
    const mx::NodePtr volume = network.terminal;
    if (!volume || volume->getCategory() != "volume") {
        return nullptr;
    }
    const mx::NodePtr vdf = MediumRewrite::connected(volume, "vdf");
    const mx::NodePtr edf = MediumRewrite::connected(volume, "edf");
    MediumRewrite rewrite(network);
    rewrite.number(vdf);
    rewrite.number(edf);
    const mx::NodeDefPtr def = mediumDefinition(network.document);
    const mx::NodePtr medium = network.document->addNodeInstance(def, network.document->createValidChildName("medium"));
    const auto set = [&medium](const char * name, const std::string & type, const std::optional<Source> & source) {
        if (source) {
            MediumRewrite::feed(medium, name, type, *source);
        }
    };
    set("absorption", "vector3", rewrite.lowered(vdf, Quantity::Absorption));
    set("scattering0", "vector3", rewrite.lowered(vdf, Quantity::Scattering, 0));
    set("anisotropy0", "float", rewrite.anisotropy(0));
    set("scattering1", "vector3", rewrite.lowered(vdf, Quantity::Scattering, 1));
    set("anisotropy1", "float", rewrite.anisotropy(1));
    set("emission", "color3", rewrite.lowered(edf, Quantity::Emission));
    return medium;
}

};  // namespace v3d::moya::material::internal
