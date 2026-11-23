/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/light/internal/LinkSets.h>

#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/light/link/LinkStep.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <vector>

namespace v3d::moya::light::internal {

LinkSets::LinkSets(const geometry::Prototypes & prototypes, const std::map<std::uint32_t, std::vector<std::uint32_t>> & naming)
    : prototypes_(prototypes), naming_(naming) {
    walk(prototypes.world.instances, 0U, 0);
}

const std::vector<std::vector<std::uint32_t>> & LinkSets::sets() const {
    return sets_;
}

void LinkSets::steps(std::vector<std::uint32_t> & first, std::vector<kernel::light::link::LinkStep> & steps) const {
    const std::uint32_t ids = steps_.empty() ? 0U : steps_.rbegin()->first + 1U;
    first.assign(static_cast<std::size_t>(ids) + 1U, 0U);
    steps.clear();
    for (const auto & [id, made] : steps_) {
        steps.insert(steps.end(), made.begin(), made.end());
        first[id + 1U] = static_cast<std::uint32_t>(steps.size());
    }
    // an instance that makes no step starts and ends where the one before it ends
    for (std::uint32_t id = 0; id < ids; ++id) {
        first[id + 1U] = std::max(first[id + 1U], first[id]);
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the set, then the depth
void LinkSets::walk(const std::vector<std::uint32_t> & members, std::uint32_t set, std::size_t depth) {
    if (depth == kernel::geometry::kInstanceDepth) {
        return;
    }
    for (const std::uint32_t member : members) {
        const geometry::Instance & instance = prototypes_.instances[member];
        const std::uint32_t next = entered(set, instance.id);
        if (instance.kind == geometry::Prototype::Group && walked_.emplace(instance.prototype, next, depth + 1).second) {
            walk(prototypes_.groups[instance.prototype].instances, next, depth + 1);
        }
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the set, then the instance
std::uint32_t LinkSets::entered(std::uint32_t set, std::uint32_t id) {
    const auto names = naming_.find(id);
    if (names == naming_.end()) {
        return set;
    }
    const std::vector<std::uint32_t> current = sets_[set];
    std::vector<std::uint32_t> merged;
    std::set_union(current.begin(), current.end(), names->second.begin(), names->second.end(), std::back_inserter(merged));
    const auto found = interned_.emplace(merged, static_cast<std::uint32_t>(sets_.size()));
    if (found.second) {
        sets_.push_back(merged);
    }
    const std::uint32_t next = found.first->second;
    std::vector<kernel::light::link::LinkStep> & made = steps_[id];
    const bool known = std::ranges::any_of(made, [&](const kernel::light::link::LinkStep & step) { return step.from == set; });
    if (next != set && !known) {
        made.push_back(kernel::light::link::LinkStep { .from = set, .to = next });
    }
    return next;
}

};  // namespace v3d::moya::light::internal
