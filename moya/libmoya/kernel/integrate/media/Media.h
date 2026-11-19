/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Layering.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/integrate/media/Enclosure.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/sample/Hash.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>

#include <cmath>
#include <cstdint>
#include <limits>

namespace v3d::moya::kernel::integrate::media {

/** The most closed surfaces a path is inside at once. **/
inline constexpr std::uint32_t kMediaDepth = 8;

/** The place in a path's media of a surface the path is not inside. **/
inline constexpr std::uint32_t kNotInside = std::numeric_limits<std::uint32_t>::max();

/**
 * The closed surfaces a path is inside, in the order it entered them. The medium a path is in is
 * the one of highest priority, and of those the one it entered last. A path inside none is in
 * the air, of index one.
 *
 * A path that enters a surface it is already inside stays as it was. One that enters a surface
 * while it is inside kMediaDepth of them leaves the one of lowest priority first, and of those
 * the one it entered first.
 **/
struct Media {
    Enclosure entries[kMediaDepth] {};
    std::uint32_t count { 0 };

    /** The place of the surface of a key, or kNotInside. **/
    MOYA_KERNEL std::uint32_t find(std::uint32_t key) const {
        for (std::uint32_t i = 0; i < count && i < kMediaDepth; ++i) {
            if (entries[i].key == key) {
                return i;
            }
        }
        return kNotInside;
    }

    /** The place of the medium the path is in, leaving out the surface of a key, or kNotInside. **/
    MOYA_KERNEL std::uint32_t winner(std::uint32_t without) const {
        std::uint32_t out = kNotInside;
        for (std::uint32_t i = 0; i < count && i < kMediaDepth; ++i) {
            if (entries[i].key != without && (out == kNotInside || entries[i].priority >= entries[out].priority)) {
                out = i;
            }
        }
        return out;
    }

    /**
     * The index of the medium that lies beyond a surface on the side its normal faces: the medium
     * the path is in, leaving that surface out.
     **/
    MOYA_KERNEL float outside(std::uint32_t key) const {
        const std::uint32_t at = winner(key);
        return at == kNotInside ? 1.0F : entries[at].index;
    }

    /** The material whose medium the path is in, or surface::kNoMaterial in the air. **/
    MOYA_KERNEL std::uint32_t medium() const {
        const std::uint32_t at = winner(kNotInside);
        return at == kNotInside ? surface::kNoMaterial : entries[at].material;
    }

    /** The key of the closed surface whose medium the path is in, or kNotInside in the air. **/
    MOYA_KERNEL std::uint32_t mediumKey() const {
        const std::uint32_t at = winner(kNotInside);
        return at == kNotInside ? kNotInside : entries[at].key;
    }

    /** Whether a hit on a surface of a priority lies inside a medium of higher priority. **/
    MOYA_KERNEL bool outranked(std::int32_t priority) const {
        const std::uint32_t at = winner(kNotInside);
        return at != kNotInside && priority < entries[at].priority;
    }

    /** Enters a closed surface. **/
    MOYA_KERNEL void enter(const Enclosure & entered) {
        if (find(entered.key) != kNotInside) {
            return;
        }
        if (count >= kMediaDepth) {
            std::uint32_t lowest = 0;
            for (std::uint32_t i = 1; i < kMediaDepth; ++i) {
                if (entries[i].priority < entries[lowest].priority) {
                    lowest = i;
                }
            }
            drop(lowest);
        }
        if (count < kMediaDepth) {
            entries[count] = entered;
            ++count;
        }
    }

    /** Leaves the closed surface of a key, when the path is inside it. **/
    MOYA_KERNEL void leave(std::uint32_t key) {
        const std::uint32_t at = find(key);
        if (at != kNotInside) {
            drop(at);
        }
    }

    /** Enters a surface crossed from the side its normal faces, and leaves one crossed back. **/
    MOYA_KERNEL void cross(const Enclosure & crossed, bool front) {
        if (front) {
            enter(crossed);
        } else {
            leave(crossed.key);
        }
    }

 private:
    MOYA_KERNEL void drop(std::uint32_t at) {
        for (std::uint32_t i = at; i + 1U < count && i + 1U < kMediaDepth; ++i) {
            entries[i] = entries[i + 1U];
        }
        --count;
    }
};

/**
 * The key of a closed surface a chain of instances places, which names them by their indices in
 * the scene, outermost first.
 **/
MOYA_KERNEL inline std::uint32_t keyOf(const std::uint32_t * chain, std::uint32_t depth) {
    std::uint64_t value = sample::mix(depth);
    for (std::uint32_t level = 0; level < depth && level < geometry::kInstanceDepth; ++level) {
        value = sample::mix(value ^ chain[level]);
    }
    return static_cast<std::uint32_t>(value);
}

/**
 * The key of the surface a hit met: the chain of instances that placed it, so every primitive of
 * one placed geometry shares it.
 **/
MOYA_KERNEL inline std::uint32_t keyOf(const geometry::Hit & hit) {
    return keyOf(hit.instances, hit.depth);
}

/** The priority of the surface a hit met: its innermost instance's that is not zero. **/
MOYA_KERNEL inline std::int32_t priorityOf(const surface::Surfaces & surfaces, const geometry::Hit & hit) {
    std::int32_t out = 0;
    for (std::uint32_t level = 0; level < hit.depth && level < geometry::kInstanceDepth; ++level) {
        const std::uint32_t id = hit.instances[level];
        if (id < surfaces.byId.size && surfaces.byId[id] != geometry::kNoPrimitive) {
            const std::int32_t own = surfaces.instances[surfaces.byId[id]].priority;
            out = own != 0 ? own : out;
        }
    }
    return out;
}

/**
 * The index of refraction of the medium a material encloses. A layered material's is its specular
 * index, scaled by its specular weight as its dielectric is. A boundary's is one, and any other
 * material's is its first dielectric closure's that is not thin.
 **/
MOYA_KERNEL inline float indexOf(const surface::Material & material) {
    if (material.boundary != 0U) {
        return 1.0F;
    }
    if (material.layered != 0U) {
        return bsdf::Layering::modulated(std::fmax(material.surface.specularIor, 1.0F), std::fmax(material.surface.specularWeight, 0.0F));
    }
    for (std::uint32_t i = 0; i < material.count && i < surface::kMostMaterialClosures; ++i) {
        if (material.closures[i].model == bsdf::Model::Dielectric && !material.closures[i].thin) {
            return material.closures[i].ior;
        }
    }
    return 1.0F;
}

/** A closed surface a hit met, of the material at a place among the surfaces' materials. **/
MOYA_KERNEL inline Enclosure enclosureOf(const surface::Surfaces & surfaces, const geometry::Hit & hit, std::uint32_t material) {
    return Enclosure { .key = keyOf(hit), .priority = priorityOf(surfaces, hit), .index = indexOf(surfaces.materials[material]), .material = material };
}

};  // namespace v3d::moya::kernel::integrate::media
