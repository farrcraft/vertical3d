/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/Ray.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::render::offline::trace {

/**
 * A bounding volume hierarchy over a set of boxes, built with a binned surface area heuristic.
 *
 * It holds the index of each box, not what the box bounds. A caller builds it from one box per
 * item, and walk() hands back the index of each item whose box a ray reaches, nearer boxes
 * first. The caller tests the item itself.
 **/
class Hierarchy final {
 public:
    /** An axis aligned box. A default box is empty, and extending it by a point takes it in. **/
    class Box final {
     public:
        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());

        void extend(const glm::vec3 & point);
        void extend(const Box & other);
        bool empty() const;
        /** Half the surface area, which is all the heuristic compares. Zero for an empty box. **/
        float area() const;
        glm::vec3 centre() const;
    };

    /**
     * Replaces the hierarchy with one over these boxes. Index i is boxes[i]. Each box is grown
     * by a small fraction of its size, so a hit that rounding puts on its face is inside it.
     **/
    void build(const std::vector<Box> & boxes);

    bool empty() const;

    /**
     * Calls `test(index)` for each item whose box the ray reaches between `from` and `*limit`,
     * nearer boxes first. The test may lower `*limit` when it finds a hit, and a box that
     * starts beyond the limit is skipped. A box that starts exactly at the limit is still
     * visited, so a caller can break a tie between two hits at one distance.
     **/
    template <typename Test>
    void walk(const v3d::type::geometry::Ray & ray, float from, const float* limit, Test test) const;

 private:
    /**
     * A node is a leaf when count is not zero, holding items order_[first] onward. An inner
     * node's first child follows it, and its second is at `second`.
     **/
    class Node final {
     public:
        Box box;
        std::uint32_t first = 0;
        std::uint32_t count = 0;
        std::uint32_t second = 0;
    };

    /** The deepest a node is put. A branch that would go deeper is a leaf. **/
    static constexpr unsigned int DEEPEST = 60;

    void split(std::uint32_t node, const std::vector<Box> & boxes, const std::vector<glm::vec3> & centres,
        unsigned int depth);

    /** A ray as the slab test reads it, and where along it the walk starts looking. **/
    class Probe final {
     public:
        glm::vec3 origin = glm::vec3(0.0F);
        glm::vec3 direction = glm::vec3(0.0F);
        /** The reciprocal of the direction, infinite along an axis the ray is parallel to. **/
        glm::vec3 inverse = glm::vec3(0.0F);
        float from = 0.0F;
    };

    /** Where along the ray it is first inside the box, from `from` on, or false if it never is. **/
    static bool enters(const Box & box, const Probe & probe, float* entry);

    /**
     * Pushes the children of an inner node that the ray reaches, the nearer one last so that
     * it is visited first. Returns the new depth of the stack.
     **/
    std::size_t descend(std::uint32_t node, const Probe & probe, std::uint32_t* stack, float* entries,
        std::size_t depth) const;

    std::vector<Node> nodes_;
    std::vector<std::uint32_t> order_;
};

template <typename Test>
void Hierarchy::walk(const v3d::type::geometry::Ray & ray, float from, const float* limit, Test test) const {
    if (nodes_.empty()) {
        return;
    }
    Probe probe;
    probe.origin = ray.origin();
    probe.direction = ray.direction();
    probe.inverse = glm::vec3(1.0F) / probe.direction;
    probe.from = from;
    float entry = 0.0F;
    if (!enters(nodes_[0].box, probe, &entry)) {
        return;
    }
    // each entry is a node and the distance the ray enters its box at
    std::uint32_t stack[DEEPEST + 2];
    float entries[DEEPEST + 2];
    stack[0] = 0;
    entries[0] = entry;
    std::size_t depth = 1;
    while (depth > 0) {
        depth--;
        if (entries[depth] > *limit) {
            continue;
        }
        const Node & node = nodes_[stack[depth]];
        if (node.count == 0) {
            depth = descend(stack[depth], probe, stack, entries, depth);
            continue;
        }
        for (std::uint32_t k = 0; k < node.count; k++) {
            test(static_cast<std::size_t>(order_[node.first + k]));
        }
    }
}

};  // namespace v3d::render::offline::trace
