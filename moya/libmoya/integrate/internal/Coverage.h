/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/film/State.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/integrate/Covered.h>
#include <moya/libmoya/output/Channel.h>
#include <moya/libmoya/output/Cryptomatte.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya::integrate::internal {

/**
 * Each pixel's coverage by each ID of a render's Cryptomatte layers, kept on the host. After each
 * pass, each pixel's records of what its samples covered are added in sample order: each ID's
 * filter weights, and the weight of every sample the pixel took. An ID's coverage is its weight
 * over that whole weight.
 *
 * A name is hashed once for each instance and material. An instance or material of no name, and
 * a sample that covered no surface, adds its weight to the whole alone.
 **/
class Coverage final {
 public:
    /** An ID a pixel holds, and the filter weights of the samples that covered it. **/
    struct Held {
        std::uint32_t id { 0 };
        double weight { 0.0 };
    };

    /** The coverage of each layer the channels name. **/
    Coverage(const glm::uvec2 & resolution, const std::vector<output::Channel> & channels, const scene::Scene & scene, const geometry::Prototypes & prototypes);

    /** Whether the render has no Cryptomatte layer, and records nothing. **/
    bool none() const;

    /** Adds what each pixel's samples covered in a pass of samples to a pixel. **/
    void add(const std::vector<kernel::integrate::Covered> & records, std::uint32_t samples);

    /**
     * Writes each pixel's IDs of the most coverage, a layer's ranks of them, into the layer's rank
     * channels in a snapshot, and each layer's manifest of the names its IDs hold. A tie keeps the
     * lower ID first, and a rank of no ID is zero.
     **/
    void into(film::Snapshot & snapshot) const;

    /** Gives a film's state every pixel's coverage, for a checkpoint, or none with no layer. **/
    void into(film::State & state) const;

    /** Continues from the coverage a film's state holds, which a checkpoint gave it. **/
    void restore(const film::State & state);

 private:
    std::size_t pixels_ { 0 };
    std::vector<output::Cryptomatte> layers_;

    /** Each layer's ID for each index a record names, and the name it was hashed from. **/
    std::vector<std::vector<std::uint32_t>> ids_;
    std::vector<std::vector<std::string>> names_;

    /** Every layer's IDs for each pixel, layer by layer, and each pixel's whole weight. **/
    std::vector<std::vector<Held>> held_;
    std::vector<double> whole_;

    /** Writes a pixel's ranks of a layer into a snapshot's rank channels, at their offsets. **/
    void rank(std::size_t layer, std::size_t pixel, const std::vector<std::uint32_t> & offsets, film::Snapshot & snapshot) const;
};

};  // namespace v3d::moya::integrate::internal
