/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/material/internal/MediumRewrite.h>

#include <moya/libmoya/kernel/volume/Medium.h>
#include <moya/libmoya/material/internal/Network.h>
#include <MaterialXCore/Definition.h>
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Interface.h>
#include <MaterialXCore/Node.h>
#include <MaterialXCore/Value.h>

#include <cstdint>
#include <optional>
#include <string>

namespace v3d::moya::material::internal {

namespace {

namespace mx = MaterialX;

constexpr const char * kZero = "0, 0, 0";

std::string typeOf(MediumRewrite::Quantity quantity) {
    return quantity == MediumRewrite::Quantity::Emission ? "color3" : "vector3";
}

MediumRewrite::Source fromNode(const mx::NodePtr & node, const std::string & output = {}) {
    return MediumRewrite::Source { .node = node, .output = output, .value = {}, .space = {} };
}

};  // namespace

mx::NodePtr MediumRewrite::connected(const mx::NodePtr & node, const std::string & name) {
    const mx::InputPtr input = node->getInput(name);
    return input ? input->getConnectedNode() : nullptr;
}

std::optional<MediumRewrite::Source> MediumRewrite::sourceOf(const mx::NodePtr & node, const std::string & name) {
    const mx::InputPtr input = node->getInput(name);
    if (input && input->getConnectedNode()) {
        return fromNode(input->getConnectedNode(), input->getOutputString());
    }
    const mx::ValuePtr value = node->getInputValue(name);
    const mx::InputPtr declared = node->getNodeDef() ? node->getNodeDef()->getActiveInput(name) : nullptr;
    if (!value || !declared) {
        return std::nullopt;
    }
    return Source { .node = nullptr, .output = {}, .value = value->getValueString(), .space = (input ? input : declared)->getActiveColorSpace() };
}

void MediumRewrite::feed(const mx::NodePtr & to, const std::string & name, const std::string & type, const Source & source) {
    if (source.node) {
        const mx::InputPtr input = to->getInput(name) ? to->getInput(name) : to->addInputFromNodeDef(name);
        input->setConnectedNode(source.node);
        if (!source.output.empty()) {
            input->setOutputString(source.output);
        }
        return;
    }
    const mx::InputPtr input = to->setInputValue(name, source.value, type);
    if (!source.space.empty()) {
        input->setColorSpace(source.space);
    }
}

void MediumRewrite::number(const mx::NodePtr & node) {
    if (!node) {
        return;
    }
    const std::string & category = node->getCategory();
    if (category == "anisotropic_vdf") {
        const auto order = static_cast<std::uint32_t>(lobes_.size());
        if (order >= kernel::volume::kMostLobes) {
            network_.report("volume shader " + node->getName() + " scatters by more than " + std::to_string(kernel::volume::kMostLobes) +
                " lobes, so the later ones scatter by the last lobe's anisotropy");
        }
        lobes_.emplace(node->getName(), order < kernel::volume::kMostLobes ? order : kernel::volume::kMostLobes - 1U);
        if (order < kernel::volume::kMostLobes) {
            leaves_.push_back(node);
        }
    } else if (category == "add") {
        number(connected(node, "in1"));
        number(connected(node, "in2"));
    } else if (category == "mix") {
        number(connected(node, "fg"));
        number(connected(node, "bg"));
    } else if (category != "absorption_vdf" && category != "uniform_edf") {
        network_.report("volume shader " + node->getName() + " is a " + category + ", which a medium does not take, so it adds nothing");
    }
}

std::optional<MediumRewrite::Source> MediumRewrite::lowered(const mx::NodePtr & node, Quantity quantity, std::uint32_t lobe) {
    if (!node) {
        return std::nullopt;
    }
    const std::string & category = node->getCategory();
    if (category == "absorption_vdf") {
        return quantity == Quantity::Absorption ? sourceOf(node, "absorption") : std::nullopt;
    }
    if (category == "anisotropic_vdf") {
        if (quantity == Quantity::Absorption) {
            return sourceOf(node, "absorption");
        }
        return quantity == Quantity::Scattering && lobes_.at(node->getName()) == lobe ? sourceOf(node, "scattering") : std::nullopt;
    }
    if (category == "uniform_edf") {
        return quantity == Quantity::Emission ? sourceOf(node, "color") : std::nullopt;
    }
    if (category == "add") {
        return added(lowered(connected(node, "in1"), quantity, lobe), lowered(connected(node, "in2"), quantity, lobe), typeOf(quantity));
    }
    if (category == "mix") {
        return mixed(lowered(connected(node, "fg"), quantity, lobe), lowered(connected(node, "bg"), quantity, lobe), sourceOf(node, "mix"),
            typeOf(quantity));
    }
    return std::nullopt;
}

std::optional<MediumRewrite::Source> MediumRewrite::anisotropy(std::uint32_t lobe) const {
    return lobe < leaves_.size() ? sourceOf(leaves_[lobe], "anisotropy") : std::nullopt;
}

mx::NodePtr MediumRewrite::made(const std::string & definition) {
    const mx::NodeDefPtr def = network_.document->getDataLibrary()->getNodeDef(definition);
    return network_.document->addNodeInstance(def, network_.document->createValidChildName("medium_" + def->getNodeString()));
}

std::optional<MediumRewrite::Source> MediumRewrite::added(const std::optional<Source> & first, const std::optional<Source> & second, const std::string & type) {
    if (!first || !second) {
        return first ? first : second;
    }
    const mx::NodePtr node = made("ND_add_" + type);
    feed(node, "in1", type, *first);
    feed(node, "in2", type, *second);
    return fromNode(node);
}

std::optional<MediumRewrite::Source> MediumRewrite::mixed(const std::optional<Source> & fg, const std::optional<Source> & bg,
    const std::optional<Source> & weight, const std::string & type) {
    if (!fg && !bg) {
        return std::nullopt;
    }
    const Source zero { .node = nullptr, .output = {}, .value = kZero, .space = {} };
    const mx::NodePtr node = made("ND_mix_" + type);
    feed(node, "fg", type, fg ? *fg : zero);
    feed(node, "bg", type, bg ? *bg : zero);
    if (weight) {
        feed(node, "mix", "float", *weight);
    }
    return fromNode(node);
}

};  // namespace v3d::moya::material::internal
