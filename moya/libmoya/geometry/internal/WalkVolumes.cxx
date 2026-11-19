/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/internal/Walk.h>

#include <moya/libmoya/geometry/Cache.h>
#include <moya/libmoya/geometry/Group.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/volume/Field.h>
#include <moya/libmoya/kernel/volume/Volume.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/volume/Grid.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::geometry::internal {

namespace {

/** The name a volume's shader reads its density by, which a volume of no program scatters. **/
constexpr const char * kDensity = "density";

};  // namespace

std::optional<std::uint32_t> Walk::volume(const scene::Handle & handle) {
    const auto found = volumeIndex_.find(handle.index);
    if (found != volumeIndex_.end()) {
        return found->second;
    }
    const std::vector<std::string> & names = scene_.value(handle, "fieldNames").stringArray();
    const std::vector<scene::Handle> & fields = scene_.value(handle, "fields").handleArray();
    if (names.size() != fields.size()) {
        report("volume " + scene_.name(handle) + " names " + std::to_string(fields.size()) + " fields and " + std::to_string(names.size()) +
            " names for them, so it reads " + std::to_string(std::min(names.size(), fields.size())));
    }
    Fields gathered { .handle = handle, .named = {} };
    std::optional<std::uint32_t> out = static_cast<std::uint32_t>(volumeFields_.size());
    for (std::size_t i = 0; i < names.size() && i < fields.size(); ++i) {
        const std::optional<std::uint32_t> read = fields[i].valid() && scene_.contains(fields[i]) ? grid(fields[i]) : std::nullopt;
        if (!read) {
            report("volume " + scene_.name(handle) + " holds nothing, because its field " + names[i] + " cannot be read");
            out = std::nullopt;
            break;
        }
        gathered.named.emplace_back(names[i], *read);
    }
    if (out) {
        volumeFields_.push_back(std::move(gathered));
    }
    volumeIndex_.emplace(handle.index, out);
    return out;
}

std::optional<std::uint32_t> Walk::grid(const scene::Handle & field) {
    const auto found = gridIndex_.find(field.index);
    if (found != gridIndex_.end()) {
        return found->second;
    }
    boost::shared_ptr<const volume::Grid> held;
    if (cache_ != nullptr) {
        const auto cached = cache_->grids_.find(field.index);
        if (cached != cache_->grids_.end()) {
            held = cached->second;
        }
    }
    if (!held) {
        const std::string & path = scene_.value(field, "filePath").stringArray().front();
        const std::string & name = scene_.value(field, "fieldName").stringArray().front();
        try {
            held = boost::make_shared<const volume::Grid>(volume::read(path, name));
        } catch (const std::runtime_error & refused) {
            report("field " + scene_.name(field) + " cannot be read, because " + refused.what());
            gridIndex_.emplace(field.index, std::nullopt);
            return std::nullopt;
        }
        if (cache_ != nullptr) {
            cache_->grids_.insert_or_assign(field.index, held);
        }
    }
    const auto out = static_cast<std::uint32_t>(prototypes_.grids.size());
    prototypes_.grids.push_back(held);
    gridIndex_.emplace(field.index, out);
    return out;
}

void Walk::placeVolumes() {
    if (volumeInstances_.empty()) {
        return;
    }
    std::map<std::uint32_t, std::vector<std::uint32_t>> byHolder;
    for (const auto & [holder, index] : volumeInstances_) {
        byHolder[holder].push_back(index);
    }
    // a group placed through two chains places its volumes through both
    struct Reached {
        const Group * group;
        std::vector<std::uint32_t> chain;
        std::uint32_t material;
        std::uint32_t visibility;
    };
    std::vector<Reached> pending { Reached { .group = &prototypes_.world, .chain = {}, .material = kernel::surface::kNoMaterial, .visibility = kernel::geometry::kEveryRay } };
    while (!pending.empty()) {
        const Reached at = std::move(pending.back());
        pending.pop_back();
        const auto held = byHolder.find(at.group->id);
        if (held != byHolder.end()) {
            for (const std::uint32_t index : held->second) {
                if (at.chain.size() >= kernel::geometry::kInstanceDepth) {
                    report("volume " + scene_.name(volumeFields_[prototypes_.instances[index].prototype].handle) + " is placed through more than " +
                        std::to_string(kernel::geometry::kInstanceDepth) + " instances, and is left out where it is");
                    continue;
                }
                placeVolume(prototypes_.instances[index], at.chain, at.material, at.visibility);
            }
        }
        for (const std::uint32_t index : at.group->instances) {
            const Instance & instance = prototypes_.instances[index];
            if (instance.kind != Prototype::Group || at.chain.size() >= kernel::geometry::kInstanceDepth) {
                continue;
            }
            Reached next { .group = &prototypes_.groups[instance.prototype],
                .chain = at.chain,
                .material = instance.material != kernel::surface::kNoMaterial ? instance.material : at.material,
                .visibility = at.visibility & instance.visibility };
            next.chain.push_back(instance.id);
            pending.push_back(std::move(next));
        }
    }
}

bool Walk::bound(const std::vector<boost::shared_ptr<const volume::Grid>> & grids, kernel::volume::Volume & placed) {
    placed.lower = glm::vec3(std::numeric_limits<float>::infinity());
    placed.upper = glm::vec3(-std::numeric_limits<float>::infinity());
    for (std::uint32_t field = 0; field < placed.fieldCount; ++field) {
        const volume::Grid & grid = *grids[placed.fields[field].grid];
        if (grid.lower.x > grid.upper.x) {
            continue;
        }
        placed.lower = glm::min(placed.lower, grid.lower);
        placed.upper = glm::max(placed.upper, grid.upper);
    }
    return placed.lower.x <= placed.upper.x && placed.lower.y <= placed.upper.y && placed.lower.z <= placed.upper.z;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the material, then the visibility
void Walk::placeVolume(const Instance & instance, std::vector<std::uint32_t> chain, std::uint32_t material, std::uint32_t visibility) {
    chain.push_back(instance.id);
    kernel::volume::Volume out;
    std::copy(chain.begin(), chain.end(), std::begin(out.chain));
    out.depth = static_cast<std::uint32_t>(chain.size());
    out.visibility = visibility & instance.visibility;
    out.material = instance.material != kernel::surface::kNoMaterial ? instance.material : material;
    const Fields & fields = volumeFields_[instance.prototype];
    const std::string name = scene_.name(fields.handle);
    const kernel::surface::Material * shaded = nullptr;
    if (out.material < prototypes_.materials.size() && prototypes_.materials[out.material].mediumProgram.size != 0U) {
        shaded = &prototypes_.materials[out.material];
    }
    const std::vector<std::string> & read = prototypes_.primvarNames;
    bool dropped = false;
    for (const auto & [named, grid] : fields.named) {
        const auto number = std::ranges::find(read, named);
        const bool density = named == kDensity;
        if (!density && (shaded == nullptr || number == read.end())) {
            continue;
        }
        if (density && shaded == nullptr) {
            out.fields[0] = kernel::volume::Field { .grid = grid, .name = kernel::volume::kUnread };
            out.fieldCount = 1;
            break;
        }
        if (out.fieldCount >= kernel::volume::kMostFields) {
            dropped = true;
            break;
        }
        if (density) {
            out.walked = out.fieldCount;
        }
        const auto numbered = number == read.end() ? kernel::volume::kUnread : static_cast<std::uint32_t>(std::distance(read.begin(), number));
        out.fields[out.fieldCount] = kernel::volume::Field { .grid = grid, .name = numbered };
        ++out.fieldCount;
    }
    if (dropped) {
        report("volume " + name + " reads more than " + std::to_string(kernel::volume::kMostFields) + " fields, so it reads the first " +
            std::to_string(kernel::volume::kMostFields));
    }
    if (out.fieldCount == 0U) {
        report(shaded == nullptr ? "volume " + name + " has no volume program and no field named density, so it holds nothing" :
                                   "volume " + name + " reads none of its fields, so it holds nothing");
        return;
    }
    out.bounded = shaded != nullptr ? shaded->mediumBounded : 1U;
    if (bound(prototypes_.grids, out)) {
        prototypes_.volumes.push_back(out);
    }
}

};  // namespace v3d::moya::geometry::internal
