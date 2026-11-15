/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/integrate/internal/Deep.h>

#include <moya/libmoya/film/DeepPoint.h>
#include <moya/libmoya/film/DeepSample.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/film/State.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/integrate/internal/Names.h>
#include <moya/libmoya/kernel/integrate/Covered.h>
#include <moya/libmoya/output/Checkpoint.h>
#include <moya/libmoya/output/Cryptomatte.h>
#include <moya/libmoya/scene/Scene.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya::integrate::internal {

namespace {

/** Each name's ID, or zero for an empty name. **/
std::vector<std::uint32_t> idsOf(const std::vector<std::string> & names) {
    std::vector<std::uint32_t> out;
    out.reserve(names.size());
    for (const std::string & name : names) {
        out.push_back(name.empty() ? 0U : output::idOf(name));
    }
    return out;
}

/** The ID an index names among ids, or zero for an index of none. **/
std::uint32_t idAt(const std::vector<std::uint32_t> & ids, std::uint32_t index) {
    return index < ids.size() ? ids[index] : 0U;
}

/** Whether a depth lies within a tolerance, a share of the other's, of an entry's depth. **/
bool near(float entry, float depth, float tolerance) {
    if (std::isinf(entry) || std::isinf(depth)) {
        return std::isinf(entry) && std::isinf(depth);
    }
    return std::fabs(depth - entry) <= tolerance * std::fabs(entry);
}

};  // namespace

Deep::Deep(const glm::uvec2 & resolution, bool wanted, float tolerance, const scene::Scene & scene, const geometry::Prototypes & prototypes) :
    pixels_(static_cast<std::size_t>(resolution.x) * resolution.y), wanted_(wanted), tolerance_(tolerance) {
    if (!wanted_) {
        return;
    }
    objectNames_ = namesOf(output::Cryptomatte::Names::Object, scene, prototypes);
    materialNames_ = namesOf(output::Cryptomatte::Names::Material, scene, prototypes);
    objects_ = idsOf(objectNames_);
    materials_ = idsOf(materialNames_);
    lists_.resize(pixels_);
    whole_.assign(pixels_, 0.0);
}

bool Deep::none() const {
    return !wanted_;
}

void Deep::join(std::size_t pixel, const kernel::integrate::Covered & record) {
    if (record.weight == 0.0F) {
        return;
    }
    whole_[pixel] += record.weight;
    const std::uint32_t object = idAt(objects_, record.object);
    const std::uint32_t material = idAt(materials_, record.material);
    std::vector<film::DeepPoint> & list = lists_[pixel];
    auto found = std::find_if(list.begin(), list.end(), [&](const film::DeepPoint & entry) {
        return entry.object == object && entry.material == material && near(entry.depth, record.depth, tolerance_);
    });
    if (found == list.end()) {
        list.push_back(film::DeepPoint { .depth = record.depth, .object = object, .material = material });
        found = list.end() - 1;
    }
    found->weight += record.weight;
    found->red += static_cast<double>(record.weight) * record.red;
    found->green += static_cast<double>(record.weight) * record.green;
    found->blue += static_cast<double>(record.weight) * record.blue;
}

void Deep::add(const std::vector<kernel::integrate::Covered> & records, std::uint32_t samples) {
    if (!wanted_) {
        return;
    }
    for (std::size_t pixel = 0; pixel < pixels_; ++pixel) {
        for (std::uint32_t sample = 0; sample < samples; ++sample) {
            join(pixel, records[(pixel * samples) + sample]);
        }
    }
}

std::vector<film::DeepSample> Deep::resolved(std::size_t pixel) const {
    std::vector<film::DeepPoint> sorted = lists_[pixel];
    std::sort(sorted.begin(), sorted.end(), [](const film::DeepPoint & left, const film::DeepPoint & right) {
        if (left.depth != right.depth) {
            return left.depth < right.depth;
        }
        return left.object != right.object ? left.object < right.object : left.material < right.material;
    });
    std::vector<film::DeepSample> out;
    double left = whole_[pixel];
    for (const film::DeepPoint & entry : sorted) {
        if (!(left > 0.0)) {
            break;
        }
        const bool behind = std::isinf(entry.depth);
        const film::DeepSample sample { .depth = entry.depth,
            .red = static_cast<float>(entry.red / left),
            .green = static_cast<float>(entry.green / left),
            .blue = static_cast<float>(entry.blue / left),
            .alpha = behind ? 0.0F : static_cast<float>(entry.weight / left),
            .object = entry.object,
            .material = entry.material };
        if (!behind || sample.red != 0.0F || sample.green != 0.0F || sample.blue != 0.0F) {
            out.push_back(sample);
        }
        left -= behind ? 0.0 : entry.weight;
    }
    return out;
}

void Deep::into(film::Snapshot & snapshot) const {
    snapshot.deep.clear();
    if (!wanted_) {
        return;
    }
    snapshot.deep.resize(pixels_);
    for (std::size_t pixel = 0; pixel < pixels_; ++pixel) {
        snapshot.deep[pixel] = resolved(pixel);
    }
    for (std::size_t i = 0; i < objectNames_.size(); ++i) {
        if (!objectNames_[i].empty()) {
            snapshot.deepObjects[objects_[i]] = objectNames_[i];
        }
    }
    for (std::size_t i = 0; i < materialNames_.size(); ++i) {
        if (!materialNames_[i].empty()) {
            snapshot.deepMaterials[materials_[i]] = materialNames_[i];
        }
    }
}

void Deep::into(film::State & state) const {
    state.deep.clear();
    if (!wanted_) {
        return;
    }
    state.deep.resize(pixels_);
    for (std::size_t pixel = 0; pixel < pixels_; ++pixel) {
        state.deep[pixel].push_back(film::DeepPoint { .depth = 0.0F, .object = film::DeepPoint::kWhole, .material = 0, .weight = whole_[pixel] });
        state.deep[pixel].insert(state.deep[pixel].end(), lists_[pixel].begin(), lists_[pixel].end());
    }
}

void Deep::restore(const film::State & state) {
    if (!wanted_) {
        return;
    }
    if (state.deep.size() != pixels_) {
        throw output::Refused("a checkpoint that holds no deep list for every pixel");
    }
    for (std::size_t pixel = 0; pixel < pixels_; ++pixel) {
        lists_[pixel].clear();
        whole_[pixel] = 0.0;
        for (const film::DeepPoint & entry : state.deep[pixel]) {
            if (entry.object == film::DeepPoint::kWhole) {
                whole_[pixel] = entry.weight;
            } else {
                lists_[pixel].push_back(entry);
            }
        }
    }
}

};  // namespace v3d::moya::integrate::internal
