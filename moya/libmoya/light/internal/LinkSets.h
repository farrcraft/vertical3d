/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/light/link/LinkStep.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <tuple>
#include <vector>

namespace v3d::moya::light::internal {

/**
 * The sets of linked lights that the chains of instances from the world reach, and the steps
 * between them. Set 0 adds nothing to the lights that light every instance. A chain that reaches
 * an instance a linked light names steps into the set that adds that light to its own. Each set
 * is the places of the lights it adds, sorted.
 *
 * The chains are walked from the world once. A group reached in the same set at the same depth
 * is walked once, and a chain deeper than kernel::geometry::kInstanceDepth is left out, as a
 * traversal leaves it out.
 **/
class LinkSets final {
 public:
    /**
     * naming holds the places of the linked lights that name each instance, sorted, by the
     * instance's index in the scene.
     **/
    LinkSets(const geometry::Prototypes & prototypes, const std::map<std::uint32_t, std::vector<std::uint32_t>> & naming);

    const std::vector<std::vector<std::uint32_t>> & sets() const;

    /**
     * The steps, each instance's from first[id] up to first[id + 1] by its index in the scene,
     * for every instance up to the last that makes one.
     **/
    void steps(std::vector<std::uint32_t> & first, std::vector<kernel::light::link::LinkStep> & steps) const;

 private:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the set, then the depth
    void walk(const std::vector<std::uint32_t> & members, std::uint32_t set, std::size_t depth);

    /** The set a chain in a set steps into at an instance, recording the step. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the set, then the instance
    std::uint32_t entered(std::uint32_t set, std::uint32_t id);

    const geometry::Prototypes & prototypes_;
    const std::map<std::uint32_t, std::vector<std::uint32_t>> & naming_;
    std::vector<std::vector<std::uint32_t>> sets_ { {} };
    std::map<std::vector<std::uint32_t>, std::uint32_t> interned_ { { {}, 0U } };
    std::map<std::uint32_t, std::vector<kernel::light::link::LinkStep>> steps_;
    std::set<std::tuple<std::uint32_t, std::uint32_t, std::size_t>> walked_;
};

};  // namespace v3d::moya::light::internal
