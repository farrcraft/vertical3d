/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/material/internal/Network.h>
#include <moya/libmoya/texture/Request.h>
#include <MaterialXCore/Node.h>
#include <MaterialXCore/Value.h>

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace v3d::moya::material::internal {

/**
 * Compiles the nodes that feed a material's surface into a program's instructions and the values
 * its registers start from. The values begin with the surface's parameters, laid out as
 * kernel::program gives them, and each constant and each node's output takes registers of its
 * own after them. A node is compiled once, however many inputs it feeds, for each place a
 * heighttonormal node reads it at.
 **/
class Emitter final {
 public:
    /**
     * @param primvars the name of each primvar a program reads, by its number, to which a name read
     *        first is added
     **/
    Emitter(Network & network, std::vector<std::string> & primvars);

    /** Holds a constant in registers from a first. A colour's first is kept with its space. **/
    void constant(std::uint32_t at, const MaterialX::ConstValuePtr & value, const std::string & type, const std::string & space);

    /** Copies a value of some components from its first register into others. **/
    void copyValue(std::uint32_t from, std::uint32_t to, std::uint32_t width);

    /**
     * The first register of a node's output, compiling the node and every node it reads. A node
     * the program does not compile, or that reads its own output, gives its output's default and
     * is reported.
     **/
    std::uint32_t output(const MaterialX::NodePtr & node, const std::string & name);

    /**
     * The first register of a node's output's default, which a node left uncompiled gives. The
     * node is reported.
     **/
    std::uint32_t uncompiled(const MaterialX::NodePtr & node, const std::string & name);

    /** Whether the program needs more registers than a shading point holds. **/
    bool overflowed() const {
        return overflowed_;
    }

    const std::vector<kernel::program::Instruction> & instructions() const {
        return instructions_;
    }

    const std::vector<float> & values() const {
        return values_;
    }

    /** The first register of each colour constant, and the space each is in. **/
    const std::vector<std::uint32_t> & colours() const {
        return colours_;
    }

    const std::vector<std::string> & spaces() const {
        return spaces_;
    }

    /** Each image an image node reads, by the number its instruction names it with. **/
    const std::vector<texture::Request> & images() const {
        return images_;
    }

 private:
    /** The first of a number of new registers. **/
    std::uint32_t allocate(std::uint32_t width);

    /** The first register of an input's value: the output that feeds it, or its constant. **/
    std::uint32_t operand(const MaterialX::NodePtr & node, const MaterialX::InputPtr & declared);

    /**
     * The instruction a node compiles to, without its output's registers, or nothing when it
     * cannot be compiled.
     **/
    std::optional<kernel::program::Instruction> instruction(const MaterialX::NodePtr & node);

    /**
     * The instruction a node that reads the shading point compiles to, or nothing when it is not
     * such a node or cannot be compiled.
     **/
    std::optional<kernel::program::Instruction> geometric(const MaterialX::NodePtr & node, std::uint32_t width);

    /**
     * The first register of the geometric property an input reads when it is given no value, such
     * as UV0 or Pobject, read once however many inputs read it. Nothing when the property is not
     * one a program reads.
     **/
    std::optional<std::uint32_t> geomprop(const MaterialX::InputPtr & declared);

    /**
     * The instruction an image node compiles to, or nothing when its output or texture coordinate
     * is not a shape the program reads. A node with no file reads no image, and gives its default.
     **/
    std::optional<kernel::program::Instruction> image(const MaterialX::NodePtr & node, std::uint32_t width);

    /**
     * The instruction a heighttonormal node compiles to, or nothing when its inputs are not the
     * shapes it reads or it feeds another one's height or texture coordinates. Its height and
     * texture coordinates are compiled three times: after a Shift along each axis of the
     * footprint, and after a Shift back to the shading point, which every instruction after it
     * reads. Each node they read is compiled again for them, apart from the same node read
     * anywhere else.
     **/
    std::optional<kernel::program::Instruction> heightToNormal(const MaterialX::NodePtr & node);

    /** Adds a Shift along an axis, which the nodes compiled after it read. **/
    void shift(std::uint32_t axis);

    /** The first register of three values of a width, combined one after another. **/
    std::uint32_t combined(const std::uint32_t (&from)[3], std::uint32_t width);

    /**
     * What each output's name is kept under: the axis the point is shifted along, whether it is
     * read for a heighttonormal node's differences, then the name.
     **/
    std::string keyed(const std::string & name) const;

    /** The value a node's input holds, or its definition's, as text. **/
    static std::string textOf(const MaterialX::NodePtr & node, const std::string & name);

    /** The number of a primvar's name, given it the first time it is read. **/
    std::uint32_t primvar(const std::string & name);

    Network & network_;
    std::vector<std::string> & primvars_;
    std::vector<kernel::program::Instruction> instructions_;
    std::vector<float> values_;
    std::vector<std::uint32_t> colours_;
    std::vector<std::string> spaces_;
    std::vector<texture::Request> images_;

    /** The first register of each output compiled, by its node's name and the output's. **/
    std::map<std::string, std::uint32_t> outputs_;

    /** The nodes whose operands are being compiled. **/
    std::set<std::string> visiting_;

    /** The axis the nodes being compiled read the shading point shifted along, or zero. **/
    std::uint32_t shift_ { 0 };

    /** Whether the nodes being compiled feed a heighttonormal node's differences. **/
    bool measuring_ { false };

    bool overflowed_ { false };
};

};  // namespace v3d::moya::material::internal
