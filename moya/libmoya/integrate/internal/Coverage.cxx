/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/integrate/internal/Coverage.h>

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Covering.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/film/State.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/integrate/Covered.h>
#include <moya/libmoya/output/Channel.h>
#include <moya/libmoya/output/Checkpoint.h>
#include <moya/libmoya/output/Cryptomatte.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya::integrate::internal {

namespace {

/** The scene's objects of a kind, by their index in the scene, and none where none is. **/
std::vector<std::optional<scene::Handle>> byIndex(const scene::Scene & scene, const std::string & kind) {
    std::vector<std::optional<scene::Handle>> out;
    for (const scene::Handle & handle : scene.objects(kind)) {
        if (handle.index >= out.size()) {
            out.resize(handle.index + 1U);
        }
        out[handle.index] = handle;
    }
    return out;
}

/** The names a layer's IDs are hashed from, by the index a record names each by. **/
std::vector<std::string> namesOf(output::Cryptomatte::Names names, const scene::Scene & scene, const geometry::Prototypes & prototypes) {
    std::vector<std::string> out;
    if (names == output::Cryptomatte::Names::Material) {
        for (const scene::Handle & handle : prototypes.materialHandles) {
            out.push_back(scene.contains(handle) ? scene.name(handle) : std::string());
        }
        return out;
    }
    const std::vector<std::optional<scene::Handle>> instances = byIndex(scene, "instance");
    out.resize(instances.size());
    for (std::size_t i = 0; i < instances.size(); ++i) {
        out[i] = instances[i] ? scene.name(*instances[i]) : std::string();
    }
    if (names == output::Cryptomatte::Names::Asset) {
        const std::vector<std::optional<scene::Handle>> groups = byIndex(scene, "group");
        for (const geometry::Instance & instance : prototypes.instances) {
            if (instance.kind != geometry::Prototype::Group || instance.id >= out.size()) {
                continue;
            }
            const std::uint32_t group = prototypes.groups[instance.prototype].id;
            if (group < groups.size() && groups[group]) {
                out[instance.id] = scene.name(*groups[group]);
            }
        }
    }
    return out;
}

};  // namespace

Coverage::Coverage(device::CpuDevice & device, const glm::uvec2 & resolution, std::uint32_t samples, const std::vector<output::Channel> & channels,
    const scene::Scene & scene, const geometry::Prototypes & prototypes) :
    device_(device), pixels_(static_cast<std::size_t>(resolution.x) * resolution.y) {
    for (const output::Channel & channel : channels) {
        if (const std::optional<output::Cryptomatte> layer = output::cryptomatteOf(channel)) {
            layers_.push_back(*layer);
            names_.push_back(namesOf(layer->names, scene, prototypes));
            std::vector<std::uint32_t> & ids = ids_.emplace_back();
            for (const std::string & name : names_.back()) {
                ids.push_back(name.empty() ? kernel::integrate::kUncovered : output::idOf(name));
            }
        }
    }
    if (layers_.empty()) {
        return;
    }
    held_.resize(layers_.size() * pixels_);
    whole_.assign(pixels_, 0.0);
    records_ = pixels_ * samples;
    const std::vector<kernel::integrate::Covered> empty(records_);
    buffer_ = device_.allocate(records_ * sizeof(kernel::integrate::Covered));
    device_.upload(*buffer_, empty.data(), records_ * sizeof(kernel::integrate::Covered));
}

bool Coverage::none() const {
    return layers_.empty();
}

kernel::Array<kernel::integrate::Covered> Coverage::view() const {
    return buffer_ ? device::view<kernel::integrate::Covered>(*buffer_) : kernel::Array<kernel::integrate::Covered> {};
}

void Coverage::add(std::uint32_t samples) {
    if (layers_.empty()) {
        return;
    }
    const std::size_t count = pixels_ * samples;
    std::vector<kernel::integrate::Covered> records(count);
    device_.download(*buffer_, records.data(), count * sizeof(kernel::integrate::Covered));
    for (std::size_t pixel = 0; pixel < pixels_; ++pixel) {
        for (std::uint32_t sample = 0; sample < samples; ++sample) {
            const kernel::integrate::Covered & record = records[(pixel * samples) + sample];
            whole_[pixel] += record.weight;
            for (std::size_t layer = 0; layer < layers_.size(); ++layer) {
                std::uint32_t index = record.object;
                if (layers_[layer].names == output::Cryptomatte::Names::Asset) {
                    index = record.asset;
                } else if (layers_[layer].names == output::Cryptomatte::Names::Material) {
                    index = record.material;
                }
                if (index >= ids_[layer].size() || ids_[layer][index] == kernel::integrate::kUncovered) {
                    continue;
                }
                std::vector<Held> & held = held_[(layer * pixels_) + pixel];
                const std::uint32_t id = ids_[layer][index];
                const auto found = std::find_if(held.begin(), held.end(), [id](const Held & each) { return each.id == id; });
                if (found == held.end()) {
                    held.push_back(Held { .id = id, .weight = record.weight });
                } else {
                    found->weight += record.weight;
                }
            }
        }
    }
}

void Coverage::rank(std::size_t layer, std::size_t pixel, const std::vector<std::uint32_t> & offsets, film::Snapshot & snapshot) const {
    std::vector<Held> ranked = held_[(layer * pixels_) + pixel];
    std::sort(ranked.begin(), ranked.end(), [](const Held & left, const Held & right) {
        return left.weight != right.weight ? left.weight > right.weight : left.id < right.id;
    });
    for (std::uint32_t at = 0; at < layers_[layer].ranks; ++at) {
        const std::size_t first = (pixel * snapshot.stride) + offsets[at / output::kRanksPerChannel] + (std::size_t { 2 } * (at % output::kRanksPerChannel));
        float * const out = &snapshot.values[first];
        out[0] = 0.0F;
        out[1] = 0.0F;
        if (at < ranked.size() && whole_[pixel] != 0.0) {
            out[0] = output::floatOf(ranked[at].id);
            out[1] = static_cast<float>(ranked[at].weight / whole_[pixel]);
        }
    }
}

void Coverage::into(film::Snapshot & snapshot) const {
    for (std::size_t layer = 0; layer < layers_.size(); ++layer) {
        const std::vector<std::string> channels = output::rankChannelsOf(layers_[layer]);
        std::vector<std::uint32_t> offsets;
        offsets.reserve(channels.size());
        for (const std::string & channel : channels) {
            offsets.push_back(snapshot.offset(channel));
        }
        std::set<std::uint32_t> seen;
        for (std::size_t pixel = 0; pixel < pixels_; ++pixel) {
            rank(layer, pixel, offsets, snapshot);
            for (const Held & each : held_[(layer * pixels_) + pixel]) {
                seen.insert(each.id);
            }
        }
        std::vector<std::string> named;
        for (std::size_t i = 0; i < names_[layer].size(); ++i) {
            if (!names_[layer][i].empty() && seen.contains(ids_[layer][i])) {
                named.push_back(names_[layer][i]);
            }
        }
        snapshot.manifests[layers_[layer].name] = output::manifestOf(named);
    }
}

void Coverage::into(film::State & state) const {
    state.coverage.clear();
    if (layers_.empty()) {
        return;
    }
    state.coverage.resize(pixels_);
    for (std::size_t pixel = 0; pixel < pixels_; ++pixel) {
        std::vector<film::Covering> & out = state.coverage[pixel];
        out.push_back(film::Covering { .layer = film::Covering::kWhole, .id = 0, .weight = whole_[pixel] });
        for (std::size_t layer = 0; layer < layers_.size(); ++layer) {
            for (const Held & held : held_[(layer * pixels_) + pixel]) {
                out.push_back(film::Covering { .layer = static_cast<std::uint32_t>(layer), .id = held.id, .weight = held.weight });
            }
        }
    }
}

void Coverage::restore(const film::State & state) {
    if (layers_.empty()) {
        return;
    }
    if (state.coverage.size() != pixels_) {
        throw output::Refused("a checkpoint that holds no Cryptomatte coverage for every pixel");
    }
    for (std::size_t pixel = 0; pixel < pixels_; ++pixel) {
        whole_[pixel] = 0.0;
        for (std::size_t layer = 0; layer < layers_.size(); ++layer) {
            held_[(layer * pixels_) + pixel].clear();
        }
        for (const film::Covering & entry : state.coverage[pixel]) {
            if (entry.layer == film::Covering::kWhole) {
                whole_[pixel] = entry.weight;
            } else if (entry.layer < layers_.size()) {
                held_[(entry.layer * pixels_) + pixel].push_back(Held { .id = entry.id, .weight = entry.weight });
            } else {
                throw output::Refused("a checkpoint whose Cryptomatte coverage names a layer the render does not have");
            }
        }
    }
}

};  // namespace v3d::moya::integrate::internal
