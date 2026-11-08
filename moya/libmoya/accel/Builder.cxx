/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Builder.h>

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Node.h>
#include <moya/libmoya/accel/internal/Binary.h>
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
Node emptyNode() {
    Node node {};
    for (std::uint32_t slot = 0; slot < kWidth; ++slot) {
        node.lowerX[slot] = node.lowerY[slot] = node.lowerZ[slot] = std::numeric_limits<float>::infinity();
        node.upperX[slot] = node.upperY[slot] = node.upperZ[slot] = -std::numeric_limits<float>::infinity();
        node.child[slot] = kEmpty;
        node.count[slot] = 0;
    }
    return node;
}

void setBox(Node & node, std::uint32_t slot, const Box & box) {
    node.lowerX[slot] = box.lower.x;
    node.lowerY[slot] = box.lower.y;
    node.lowerZ[slot] = box.lower.z;
    node.upperX[slot] = box.upper.x;
    node.upperY[slot] = box.upper.y;
    node.upperZ[slot] = box.upper.z;
}

Box boxOf(const Node & node, std::uint32_t slot) {
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
    while (children.size() < kWidth) {
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
            link = kLeaf | static_cast<std::uint32_t>(out.references.size());
            count = child.count;
            out.references.insert(out.references.end(), binary.references.begin() + child.first,
                binary.references.begin() + child.first + child.count);
        } else {
            link = collapse(binary, children[slot], out);
        }
        // the vector may have grown while the child was added, so the node is found again
        Node & node = out.nodes[index];
        setBox(node, slot, child.box);
        node.child[slot] = link;
        node.count[slot] = count;
    }
    return index;
}

/** A node's cost and its children's, over the root's area. **/
double nodeCost(const Hierarchy & hierarchy, std::uint32_t at, const Box & box, double rootArea) {
    double total = static_cast<double>(box.area()) / rootArea;
    const Node & node = hierarchy.nodes[at];
    for (std::uint32_t slot = 0; slot < kWidth; ++slot) {
        const std::uint32_t child = node.child[slot];
        if (child == kEmpty) {
            continue;
        }
        const Box childBox = boxOf(node, slot);
        if ((child & kLeaf) != 0) {
            total += (static_cast<double>(childBox.area()) / rootArea) * static_cast<double>(node.count[slot]);
        } else {
            total += nodeCost(hierarchy, child, childBox, rootArea);
        }
    }
    return total;
}

};  // namespace

Hierarchy build(device::CpuDevice & device, const std::vector<Box> & bounds, const Clip & clip, const Options & options) {
    if (!std::isfinite(options.budget) || options.budget < 0.0F) {
        throw std::invalid_argument("a budget of spatial splits must be finite and not negative");
    }
    if (bounds.size() >= kLeaf) {
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
    const double room = static_cast<double>(kLeaf - references.size());
    const double wanted = std::min(std::floor(static_cast<double>(options.budget) * static_cast<double>(references.size())), room);
    const auto budget = static_cast<std::size_t>(wanted);  // checked: finite, and 0 to the room a leaf names
    const internal::BinaryBuilder builder(device, clip, options, out.bounds.area());
    const internal::Binary binary = builder.build(std::move(references), out.bounds, budget);
    if (binary.nodes.front().leaf) {
        // a hierarchy of one leaf still has a root node, which holds it
        Node root = emptyNode();
        setBox(root, 0, binary.nodes.front().box);
        root.child[0] = kLeaf;
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

};  // namespace v3d::moya::accel
