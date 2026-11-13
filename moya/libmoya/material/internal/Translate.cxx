/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/material/internal/Translate.h>

#include <moya/libmoya/material/internal/Network.h>
#include <moya/libmoya/material/internal/Translation.h>
#include <MaterialXCore/Definition.h>
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Interface.h>
#include <MaterialXCore/Node.h>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

namespace v3d::moya::material::internal {

namespace {

namespace mx = MaterialX;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";

/** The suffix each output of a translation has after the OpenPBR input it feeds. **/
constexpr std::string_view kOut = "_out";

using Passed = Translation::Passed;

constexpr std::array kTranslations {
    Translation { .definition = "ND_standard_surface_surfaceshader",
        .translator = "ND_standard_surface_to_open_pbr_surface",
        .passed = { Passed { .from = "normal", .to = "geometry_normal" }, Passed { .from = "tangent", .to = "geometry_tangent" },
            Passed { .from = "coat_normal", .to = "geometry_coat_normal" }, Passed { .from = "thin_walled", .to = "geometry_thin_walled" } } },
    Translation { .definition = "ND_UsdPreviewSurface_surfaceshader", .translator = "ND_UsdPreviewSurface_to_open_pbr_surface", .passed = {} },
    Translation { .definition = "ND_gltf_pbr_surfaceshader",
        .translator = "ND_gltf_pbr_to_open_pbr_surface",
        .passed = { Passed { .from = "normal", .to = "geometry_normal" }, Passed { .from = "tangent", .to = "geometry_tangent" },
            Passed { .from = "clearcoat_normal", .to = "geometry_coat_normal" }, Passed {} } },
};

/** The translation of a definition, or none. **/
const Translation * translationOf(const std::string & definition) {
    for (const Translation & translation : kTranslations) {
        if (definition == translation.definition) {
            return &translation;
        }
    }
    return nullptr;
}

/**
 * Gives an input of a node what an input of another gives: the node and the output that feed it,
 * or its value with its colour space. Nothing when the other gives neither.
 **/
void copyInput(const mx::NodePtr & to, const mx::InputPtr & declared, const mx::InputPtr & from) {
    if (!from) {
        return;
    }
    const mx::NodePtr feeding = from->getConnectedNode();
    if (feeding) {
        const mx::InputPtr input = to->addInputFromNodeDef(declared->getName());
        input->removeAttribute(mx::ValueElement::VALUE_ATTRIBUTE);
        input->setConnectedNode(feeding);
        if (from->hasOutputString()) {
            input->setOutputString(from->getOutputString());
        }
        return;
    }
    if (!from->hasValueString()) {
        return;
    }
    const mx::InputPtr input = to->setInputValue(declared->getName(), from->getValueString(), declared->getType());
    if (from->hasColorSpace()) {
        input->setColorSpace(from->getColorSpace());
    }
}

};  // namespace

MaterialX::NodePtr translated(Network & network, const MaterialX::NodePtr & surface) {
    const mx::NodeDefPtr def = surface->getNodeDef();
    const Translation * translation = def ? translationOf(def->getName()) : nullptr;
    if (translation == nullptr) {
        return surface;
    }
    const mx::DocumentPtr & document = network.document;
    const mx::NodeDefPtr translator = document->getDataLibrary()->getNodeDef(translation->translator);
    const mx::NodeDefPtr openPbr = document->getDataLibrary()->getNodeDef(kOpenPbr);
    if (!translator || !openPbr) {
        network.report(std::string("the libraries hold no ") + translation->translator + ", so " + surface->getName() + " is not translated onto OpenPBR");
        return surface;
    }
    const mx::NodePtr translating = document->addNodeInstance(translator, document->createValidChildName(surface->getName() + "_translation"));
    for (const mx::InputPtr & declared : translator->getActiveInputs()) {
        const mx::InputPtr given = surface->getInput(declared->getName());
        copyInput(translating, declared, given ? given : def->getActiveInput(declared->getName()));
    }
    const mx::NodePtr out = document->addNodeInstance(openPbr, document->createValidChildName(surface->getName() + "_open_pbr"));
    for (const mx::OutputPtr & output : translator->getActiveOutputs()) {
        const std::string & name = output->getName();
        if (name.size() <= kOut.size() || !name.ends_with(kOut)) {
            continue;
        }
        const std::string target = name.substr(0, name.size() - kOut.size());
        const bool passes = std::ranges::any_of(translation->passed, [&target](const Passed & passed) {
            return passed.to != nullptr && target == passed.to;
        });
        if (passes || !openPbr->getActiveInput(target)) {
            continue;
        }
        const mx::InputPtr input = out->addInputFromNodeDef(target);
        input->removeAttribute(mx::ValueElement::VALUE_ATTRIBUTE);
        input->setConnectedNode(translating);
        input->setOutputString(name);
    }
    for (const Passed & passed : translation->passed) {
        if (passed.from == nullptr) {
            continue;
        }
        const mx::InputPtr given = surface->getInput(passed.from);
        const mx::InputPtr declared = openPbr->getActiveInput(passed.to);
        // a normal or a tangent no node feeds is the shading point's own, as OpenPBR's default is
        if (declared && given && (given->getConnectedNode() || declared->getType() == "boolean")) {
            copyInput(out, declared, given);
        }
    }
    document->removeNode(surface->getName());
    return out;
}

};  // namespace v3d::moya::material::internal
