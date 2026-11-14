/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

namespace v3d::moya::kernel::film {

/**
 * How a channel's samples are combined into its pixel.
 *
 * - Mean is the mean of the samples weighted by the pixel filter, as the beauty is.
 * - Sum is the sum of the samples, without the filter's weights.
 * - Closest is the sample nearest the camera, by its key, the camera ray's depth. Of samples at
 *   the same depth it keeps the one of the lowest index.
 * - Least and Most are each component's least and greatest over the samples.
 *
 * A pixel no sample reached holds zero, whatever its channel's combine.
 **/
enum class Combine : std::uint8_t {
    Mean,
    Sum,
    Closest,
    Least,
    Most,
};

/** The floats a channel takes in a pixel: its components, and the key of a closest one. **/
MOYA_KERNEL constexpr std::uint32_t floatsOf(Combine combine, std::uint32_t components) {
    return combine == Combine::Closest ? components + 1U : components;
}

};  // namespace v3d::moya::kernel::film
