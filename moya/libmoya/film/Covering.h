/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::film {

/**
 * One entry of a pixel's Cryptomatte coverage, as a checkpoint keeps it: a layer, by its place
 * among the render's layers, an ID, and the filter weights of the samples that covered it. An
 * entry of the layer kWhole holds the weight of every sample the pixel took, and its ID is zero.
 **/
struct Covering {
    /** The layer of the entry that holds a pixel's whole weight. **/
    static constexpr std::uint32_t kWhole = 0xFFFFFFFFU;

    std::uint32_t layer { 0 };
    std::uint32_t id { 0 };
    double weight { 0.0 };
};

};  // namespace v3d::moya::film
