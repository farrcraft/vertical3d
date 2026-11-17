/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/material/internal/Network.h>
#include <MaterialXCore/Node.h>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace v3d::moya::material::internal {

/**
 * Rewrites one volume's VDF and EDF trees into the ordinary nodes that feed a medium's values, as
 * rewrittenMedium() describes. Each anisotropic_vdf is numbered by its lobe first, and each value
 * is then lowered from the tree's root.
 **/
class MediumRewrite final {
 public:
    /** What feeds an input: another node's output, or a value with the colour space it is in. **/
    struct Source {
        MaterialX::NodePtr node;
        std::string output;
        std::string value;
        std::string space;
    };

    /** What a medium's value is: the absorption, a lobe's scattering, or the emission. **/
    enum class Quantity : std::uint8_t { Absorption, Scattering, Emission };

    explicit MediumRewrite(Network & network) : network_(network) {}

    /**
     * Numbers every anisotropic_vdf under a node by its lobe, and reports each node a medium does
     * not take.
     **/
    void number(const MaterialX::NodePtr & node);

    /** What feeds a medium's value from a node of a VDF or an EDF tree, or nothing for none. **/
    std::optional<Source> lowered(const MaterialX::NodePtr & node, Quantity quantity, std::uint32_t lobe = 0);

    /** What feeds a lobe's anisotropy: its leaf's, or nothing for a lobe no leaf scatters by. **/
    std::optional<Source> anisotropy(std::uint32_t lobe) const;

    /** The node an input of a node is connected to, or none. **/
    static MaterialX::NodePtr connected(const MaterialX::NodePtr & node, const std::string & name);

    /**
     * What feeds an input of a node: what it is connected to, or its value, or its definition's.
     **/
    static std::optional<Source> sourceOf(const MaterialX::NodePtr & node, const std::string & name);

    /** Feeds an input of a node, of a MaterialX type, from a source. **/
    static void feed(const MaterialX::NodePtr & to, const std::string & name, const std::string & type, const Source & source);

 private:
    /** A new node of a definition the libraries hold. **/
    MaterialX::NodePtr made(const std::string & definition);

    /** The sum of two sources, or either alone. **/
    std::optional<Source> added(const std::optional<Source> & first, const std::optional<Source> & second, const std::string & type);

    /** A mix of two sources by a weight, with nothing as zero, or nothing for neither. **/
    std::optional<Source> mixed(const std::optional<Source> & fg, const std::optional<Source> & bg, const std::optional<Source> & weight,
        const std::string & type);

    Network & network_;

    /** The lobe of each anisotropic_vdf, by its node's name. **/
    std::map<std::string, std::uint32_t> lobes_;

    /** The leaf whose anisotropy each lobe scatters by. **/
    std::vector<MaterialX::NodePtr> leaves_;
};

};  // namespace v3d::moya::material::internal
