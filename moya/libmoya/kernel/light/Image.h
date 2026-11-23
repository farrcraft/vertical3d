/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::light {

/**
 * An image a light reads, its texels held among the lights' texels from texels, row after row
 * from its top, in ACEScg.
 *
 * A dome light's image also holds two ways of drawing a texel, each a row then a texel along it,
 * as the sums of their chances among the lights' sums. rows is the first of height sums for the
 * rows, and columns the first of width sums for each row, row after row. The first way draws a
 * texel in proportion to its luminance times the solid angle it covers. The second takes the
 * image's average luminance off each texel first, held to zero or more, as Karlik and others'
 * multiple importance sampling compensation does, from compensatedRows and compensatedColumns.
 * An image no dome light reads holds no ways of drawing.
 **/
struct Image {
    std::uint32_t texels { 0 };
    std::uint32_t width { 0 };
    std::uint32_t height { 0 };
    std::uint32_t rows { 0 };
    std::uint32_t columns { 0 };
    std::uint32_t compensatedRows { 0 };
    std::uint32_t compensatedColumns { 0 };
};

};  // namespace v3d::moya::kernel::light
