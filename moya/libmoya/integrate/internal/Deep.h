/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/film/DeepPoint.h>
#include <moya/libmoya/film/DeepSample.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/film/State.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/integrate/Covered.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya::integrate::internal {

/**
 * Each pixel's deep list, kept on the host. After each pass, each pixel's records of what its
 * samples covered are joined in sample order. A record joins the first entry of its object and
 * material whose depth lies within the tolerance, a share of that entry's depth, of its own, and
 * starts an entry otherwise. Records of no surface join the entry of their depth in the same way,
 * and every record that met nothing joins one entry behind all the others. A record of a collision
 * in a medium joins only an entry of collisions, which spreads from the least depth its records
 * hold to the most.
 *
 * An entry resolves to a sample from its depth to its back, a point for a surface's. Its alpha is
 * its weight over the weight of the samples that reached its depth, and its colour its weighted
 * radiance over that weight. Compositing a pixel's samples front to back then gives its flat
 * beauty and alpha. The entry of what met nothing has an alpha of zero, and is left out when it
 * holds no light.
 **/
class Deep final {
 public:
    /** A deep list for each pixel when wanted is true, joined within the tolerance given. **/
    Deep(const glm::uvec2 & resolution, bool wanted, float tolerance, const scene::Scene & scene, const geometry::Prototypes & prototypes);

    /** Whether the render writes no deep file, and keeps no lists. **/
    bool none() const;

    /** Joins each pixel's records of a pass of samples to a pixel into its list. **/
    void add(const std::vector<kernel::integrate::Covered> & records, std::uint32_t samples);

    /** Gives a snapshot each pixel's samples, front to back, and the names of their IDs. **/
    void into(film::Snapshot & snapshot) const;

    /** Gives a film's state every pixel's list, for a checkpoint, or none with no lists. **/
    void into(film::State & state) const;

    /** Continues from the lists a film's state holds, which a checkpoint gave it. **/
    void restore(const film::State & state);

 private:
    std::size_t pixels_ { 0 };
    bool wanted_ { false };
    float tolerance_ { 0.0F };

    /** The ID of each object and material a record names, and the name it was hashed from. **/
    std::vector<std::uint32_t> objects_;
    std::vector<std::uint32_t> materials_;
    std::vector<std::string> objectNames_;
    std::vector<std::string> materialNames_;

    /** Each pixel's entries, in the order they were started, and its whole weight. **/
    std::vector<std::vector<film::DeepPoint>> lists_;
    std::vector<double> whole_;

    /** Joins one record into a pixel's list. **/
    void join(std::size_t pixel, const kernel::integrate::Covered & record);

    /** A pixel's samples, its entries resolved front to back. **/
    std::vector<film::DeepSample> resolved(std::size_t pixel) const;
};

};  // namespace v3d::moya::integrate::internal
