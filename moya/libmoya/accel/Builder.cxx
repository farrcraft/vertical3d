/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Builder.h>

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/device/cpu/MotionNode.h>
#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/accel/Hierarchy.h>
#include <moya/libmoya/accel/MovingHierarchy.h>
#include <moya/libmoya/accel/Options.h>
#include <moya/libmoya/accel/internal/Binary.h>
#include <moya/libmoya/accel/internal/BinaryBuilder.h>
#include <moya/libmoya/accel/internal/BinaryNode.h>
#include <moya/libmoya/accel/internal/MovingBinary.h>
#include <moya/libmoya/accel/internal/MovingBinaryNode.h>
#include <moya/libmoya/accel/internal/MovingReference.h>
#include <moya/libmoya/accel/internal/Reference.h>
#include <moya/libmoya/accel/internal/TemporalBuilder.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/device/CpuDevice.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace v3d::moya::accel {

namespace {

/** A node whose every slot is empty. **/
device::cpu::Node emptyNode() {
    device::cpu::Node node {};
    for (std::uint32_t slot = 0; slot < device::cpu::kWidth; ++slot) {
        node.lowerX[slot] = node.lowerY[slot] = node.lowerZ[slot] = std::numeric_limits<float>::infinity();
        node.upperX[slot] = node.upperY[slot] = node.upperZ[slot] = -std::numeric_limits<float>::infinity();
        node.child[slot] = device::cpu::kEmpty;
        node.count[slot] = 0;
    }
    return node;
}

void setBox(device::cpu::Node & node, std::uint32_t slot, const Box & box) {
    node.lowerX[slot] = box.lower.x;
    node.lowerY[slot] = box.lower.y;
    node.lowerZ[slot] = box.lower.z;
    node.upperX[slot] = box.upper.x;
    node.upperY[slot] = box.upper.y;
    node.upperZ[slot] = box.upper.z;
}

Box boxOf(const device::cpu::Node & node, std::uint32_t slot) {
    return Box { .lower = { node.lowerX[slot], node.lowerY[slot], node.lowerZ[slot] },
        .upper = { node.upperX[slot], node.upperY[slot], node.upperZ[slot] } };
}

/**
 * The children a binary node's wide node holds. They start as its own two. The child of the
 * largest area that is not a leaf is then opened in its place, until there are eight or every
 * child is a leaf.
 **/
std::vector<std::uint32_t> gathered(const internal::Binary & binary, std::uint32_t at) {
    std::vector<std::uint32_t> children { binary.nodes[at].left, binary.nodes[at].right };
    while (children.size() < device::cpu::kWidth) {
        std::size_t widest = children.size();
        float area = -1.0F;
        for (std::size_t i = 0; i < children.size(); ++i) {
            const internal::BinaryNode & child = binary.nodes[children[i]];
            if (!child.leaf && child.box.area() > area) {
                widest = i;
                area = child.box.area();
            }
        }
        if (widest == children.size()) {
            break;
        }
        const internal::BinaryNode & opened = binary.nodes[children[widest]];
        children[widest] = opened.left;
        children.insert(children.begin() + static_cast<std::ptrdiff_t>(widest) + 1, opened.right);
    }
    return children;
}

/** Adds the wide node of a binary node that is not a leaf, and those below. Gives its index. **/
std::uint32_t collapse(const internal::Binary & binary, std::uint32_t at, Hierarchy & out) {
    const auto index = static_cast<std::uint32_t>(out.nodes.size());
    out.nodes.push_back(emptyNode());
    const std::vector<std::uint32_t> children = gathered(binary, at);
    for (std::uint32_t slot = 0; slot < children.size(); ++slot) {
        const internal::BinaryNode & child = binary.nodes[children[slot]];
        std::uint32_t link = 0;
        std::uint32_t count = 0;
        if (child.leaf) {
            link = device::cpu::kLeaf | static_cast<std::uint32_t>(out.references.size());
            count = child.count;
            out.references.insert(out.references.end(), binary.references.begin() + child.first,
                binary.references.begin() + child.first + child.count);
        } else {
            link = collapse(binary, children[slot], out);
        }
        // the vector may have grown while the child was added, so the node is found again
        device::cpu::Node & node = out.nodes[index];
        setBox(node, slot, child.box);
        node.child[slot] = link;
        node.count[slot] = count;
    }
    return index;
}

/** A node's cost and its children's, over the root's area. **/
double nodeCost(const Hierarchy & hierarchy, std::uint32_t at, const Box & box, double rootArea) {
    double total = static_cast<double>(box.area()) / rootArea;
    const device::cpu::Node & node = hierarchy.nodes[at];
    for (std::uint32_t slot = 0; slot < device::cpu::kWidth; ++slot) {
        const std::uint32_t child = node.child[slot];
        if (child == device::cpu::kEmpty) {
            continue;
        }
        const Box childBox = boxOf(node, slot);
        if ((child & device::cpu::kLeaf) != 0) {
            total += (static_cast<double>(childBox.area()) / rootArea) * static_cast<double>(node.count[slot]);
        } else {
            total += nodeCost(hierarchy, child, childBox, rootArea);
        }
    }
    return total;
}


/** A moving node whose every slot is empty, over no time. **/
device::cpu::MotionNode emptyMotionNode() {
    device::cpu::MotionNode node {};
    for (std::uint32_t slot = 0; slot < device::cpu::kWidth; ++slot) {
        for (float * plane : { node.lowerX, node.lowerY, node.lowerZ, node.endLowerX, node.endLowerY, node.endLowerZ }) {
            plane[slot] = std::numeric_limits<float>::infinity();
        }
        for (float * plane : { node.upperX, node.upperY, node.upperZ, node.endUpperX, node.endUpperY, node.endUpperZ }) {
            plane[slot] = -std::numeric_limits<float>::infinity();
        }
        node.begin[slot] = std::numeric_limits<float>::infinity();
        node.end[slot] = -std::numeric_limits<float>::infinity();
        node.child[slot] = device::cpu::kEmpty;
        node.count[slot] = 0;
    }
    return node;
}

/** Sets a slot's planes at both ends of its range. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the range's begin, then its end
void setMoving(device::cpu::MotionNode & node, std::uint32_t slot, const LinearBox & box, float begin, float end) {
    node.lowerX[slot] = box.start.lower.x;
    node.lowerY[slot] = box.start.lower.y;
    node.lowerZ[slot] = box.start.lower.z;
    node.upperX[slot] = box.start.upper.x;
    node.upperY[slot] = box.start.upper.y;
    node.upperZ[slot] = box.start.upper.z;
    node.endLowerX[slot] = box.end.lower.x;
    node.endLowerY[slot] = box.end.lower.y;
    node.endLowerZ[slot] = box.end.lower.z;
    node.endUpperX[slot] = box.end.upper.x;
    node.endUpperY[slot] = box.end.upper.y;
    node.endUpperZ[slot] = box.end.upper.z;
    node.begin[slot] = begin;
    node.end[slot] = end;
}

/** A moving node's slot's bounds over its range. **/
LinearBox linearOf(const device::cpu::MotionNode & node, std::uint32_t slot) {
    return LinearBox { .start = Box { .lower = { node.lowerX[slot], node.lowerY[slot], node.lowerZ[slot] },
                           .upper = { node.upperX[slot], node.upperY[slot], node.upperZ[slot] } },
        .end = Box { .lower = { node.endLowerX[slot], node.endLowerY[slot], node.endLowerZ[slot] },
            .upper = { node.endUpperX[slot], node.endUpperY[slot], node.endUpperZ[slot] } } };
}

/** A moving binary node's weight as a child to open: its area times its share of the time. **/
float weightOf(const internal::MovingBinaryNode & node, float span) {
    return span > 0.0F ? node.box.area() * ((node.end - node.begin) / span) : node.box.area();
}

/** The children a moving binary node's wide node holds, gathered as gathered() gathers them. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the node, then the hierarchy's time span
std::vector<std::uint32_t> gatheredMoving(const internal::MovingBinary & binary, std::uint32_t at, float span) {
    std::vector<std::uint32_t> children { binary.nodes[at].left, binary.nodes[at].right };
    while (children.size() < device::cpu::kWidth) {
        std::size_t widest = children.size();
        float weight = -1.0F;
        for (std::size_t i = 0; i < children.size(); ++i) {
            const internal::MovingBinaryNode & child = binary.nodes[children[i]];
            if (!child.leaf && weightOf(child, span) > weight) {
                widest = i;
                weight = weightOf(child, span);
            }
        }
        if (widest == children.size()) {
            break;
        }
        const internal::MovingBinaryNode & opened = binary.nodes[children[widest]];
        children[widest] = opened.left;
        children.insert(children.begin() + static_cast<std::ptrdiff_t>(widest) + 1, opened.right);
    }
    return children;
}

/** Adds the wide node of a moving binary node that is not a leaf, and those below it. **/
std::uint32_t collapseMoving(const internal::MovingBinary & binary, std::uint32_t at, float span, MovingHierarchy & out) {
    const auto index = static_cast<std::uint32_t>(out.nodes.size());
    out.nodes.push_back(emptyMotionNode());
    const std::vector<std::uint32_t> children = gatheredMoving(binary, at, span);
    for (std::uint32_t slot = 0; slot < children.size(); ++slot) {
        const internal::MovingBinaryNode & child = binary.nodes[children[slot]];
        std::uint32_t link = 0;
        std::uint32_t count = 0;
        if (child.leaf) {
            link = device::cpu::kLeaf | static_cast<std::uint32_t>(out.references.size());
            count = child.count;
            out.references.insert(out.references.end(), binary.references.begin() + child.first,
                binary.references.begin() + child.first + child.count);
        } else {
            link = collapseMoving(binary, children[slot], span, out);
        }
        // the vector may have grown while the child was added, so the node is found again
        device::cpu::MotionNode & node = out.nodes[index];
        setMoving(node, slot, child.box, child.begin, child.end);
        node.child[slot] = link;
        node.count[slot] = count;
    }
    return index;
}

/** A moving node's children's cost, over the root's area, each weighted by its share of time. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the root's area, then its time span
double movingCost(const MovingHierarchy & hierarchy, std::uint32_t at, double rootArea, double span) {
    double total = 0.0;
    const device::cpu::MotionNode & node = hierarchy.nodes[at];
    for (std::uint32_t slot = 0; slot < device::cpu::kWidth; ++slot) {
        const std::uint32_t child = node.child[slot];
        if (child == device::cpu::kEmpty) {
            continue;
        }
        const double share = span > 0.0 ? static_cast<double>(node.end[slot] - node.begin[slot]) / span : 1.0;
        const double weight = (static_cast<double>(linearOf(node, slot).area()) / rootArea) * share;
        if ((child & device::cpu::kLeaf) != 0) {
            total += weight * static_cast<double>(node.count[slot]);
        } else {
            total += weight + movingCost(hierarchy, child, rootArea, span);
        }
    }
    return total;
}

};  // namespace

Hierarchy build(device::CpuDevice & device, const std::vector<Box> & bounds, const Clip & clip, const Options & options) {
    if (!std::isfinite(options.budget) || options.budget < 0.0F) {
        throw std::invalid_argument("a budget of spatial splits must be finite and not negative");
    }
    if (bounds.size() >= device::cpu::kLeaf) {
        throw std::invalid_argument("a hierarchy holds fewer primitives than a leaf's first reference can name");
    }
    Hierarchy out;
    std::vector<internal::Reference> references;
    references.reserve(bounds.size());
    for (std::size_t i = 0; i < bounds.size(); ++i) {
        if (!bounds[i].empty()) {
            references.push_back(internal::Reference { .box = bounds[i], .primitive = static_cast<std::uint32_t>(i) });
            out.bounds.grow(bounds[i]);
        }
    }
    if (references.empty()) {
        out.nodes.push_back(emptyNode());
        return out;
    }
    // the references a leaf can name bound how many spatial splits may add
    const double room = static_cast<double>(device::cpu::kLeaf - references.size());
    const double wanted = std::min(std::floor(static_cast<double>(options.budget) * static_cast<double>(references.size())), room);
    const auto budget = static_cast<std::size_t>(wanted);  // checked: finite, and 0 to the room a leaf names
    const internal::BinaryBuilder builder(device, clip, options, out.bounds.area());
    const internal::Binary binary = builder.build(std::move(references), out.bounds, budget);
    if (binary.nodes.front().leaf) {
        // a hierarchy of one leaf still has a root node, which holds it
        device::cpu::Node root = emptyNode();
        setBox(root, 0, binary.nodes.front().box);
        root.child[0] = device::cpu::kLeaf;
        root.count[0] = binary.nodes.front().count;
        out.nodes.push_back(root);
        out.references = binary.references;
        return out;
    }
    collapse(binary, 0, out);
    return out;
}

double cost(const Hierarchy & hierarchy) {
    const double rootArea = hierarchy.bounds.area();
    if (!(rootArea > 0.0)) {
        return 0.0;
    }
    return nodeCost(hierarchy, 0, hierarchy.bounds, rootArea);
}

MovingHierarchy buildMoving(device::CpuDevice & device, std::uint32_t count, const Sweep & sweep, float begin, float end, const Options & options) {
    if (!std::isfinite(begin) || !std::isfinite(end) || end < begin) {
        throw std::invalid_argument("a moving hierarchy's time range must be finite and end at or after it begins");
    }
    if (count >= device::cpu::kLeaf) {
        throw std::invalid_argument("a hierarchy holds fewer primitives than a leaf's first reference can name");
    }
    MovingHierarchy out;
    out.begin = begin;
    out.end = end;
    std::vector<internal::MovingReference> references;
    references.reserve(count);
    for (std::uint32_t primitive = 0; primitive < count; ++primitive) {
        const LinearBox box = sweep(primitive, begin, end);
        if (!box.empty()) {
            references.push_back(internal::MovingReference { .box = box, .primitive = primitive });
            out.bounds.grow(box);
        }
    }
    if (references.empty()) {
        out.nodes.push_back(emptyMotionNode());
        return out;
    }
    const internal::TemporalBuilder builder(device, sweep, options);
    const internal::MovingBinary binary = builder.build(std::move(references), begin, end);
    if (binary.nodes.front().leaf) {
        // a hierarchy of one leaf still has a root node, which holds it
        device::cpu::MotionNode root = emptyMotionNode();
        setMoving(root, 0, binary.nodes.front().box, begin, end);
        root.child[0] = device::cpu::kLeaf;
        root.count[0] = binary.nodes.front().count;
        out.nodes.push_back(root);
        out.references = binary.references;
        return out;
    }
    collapseMoving(binary, 0, end - begin, out);
    return out;
}

double cost(const MovingHierarchy & hierarchy) {
    const double rootArea = hierarchy.bounds.area();
    if (!(rootArea > 0.0)) {
        return 0.0;
    }
    return 1.0 + movingCost(hierarchy, 0, rootArea, static_cast<double>(hierarchy.end) - hierarchy.begin);
}

};  // namespace v3d::moya::accel
