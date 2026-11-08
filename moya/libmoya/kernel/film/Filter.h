/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>
#include <cstdint>

#include <glm/vec2.hpp>

namespace v3d::moya::kernel::film {

/** How a pixel filter draws a position: by its own inverse, or from a table of its values. **/
enum class FilterKind : std::uint8_t {
    Box,
    Triangle,
    Table
};

/** A camera sample's offset from its pixel's centre, in pixels, and the weight it carries. **/
struct FilterSample {
    glm::vec2 offset { 0.0F };
    float weight { 0.0F };
};

/**
 * A pixel filter, which draws each camera sample's offset from the pixel's centre with a density
 * that follows the filter. A sample's weight is the filter's value over that density, so a
 * pixel's weighted mean of its own samples is the filtered picture.
 *
 * A box draws uniformly, and a triangle by its inverse. Every other filter is a table of its
 * values at the centres of a grid of cells over its extent, with the cumulative distribution of
 * the rows' absolute values and of each row's cells. A cell is drawn in proportion to its absolute
 * value, and a position within it uniformly. Its weight is the integral of the absolute value,
 * with the cell's sign, so a filter with negative lobes gives samples of either sign.
 **/
struct PixelFilter {
    FilterKind kind { FilterKind::Box };

    /** Half the filter's extent along each axis, in pixels. **/
    glm::vec2 radius { 0.5F };

    /** A table's values, one per cell, row by row from the top left of the extent. **/
    Array<float> values;

    /** The cumulative distribution over a table's rows, of rows + 1 entries. **/
    Array<float> rowCdf;

    /** The cumulative distribution over each row's cells, columns + 1 entries per row. **/
    Array<float> cellCdf;

    std::uint32_t columns { 0 };
    std::uint32_t rows { 0 };

    /** The integral of a table's absolute value over its extent. **/
    float integral { 0.0F };

    /**
     * The index i in [0, count) with cdf[i] <= u < cdf[i + 1], for a cumulative distribution
     * of count + 1 entries that rises from cdf[0] = 0 to cdf[count] = 1, and u in [0, 1). An
     * interval of no width is never found.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the table, its length, the point
    MOYA_KERNEL static std::uint32_t findInterval(const float * cdf, std::uint32_t count, float u) {
        std::uint32_t low = 0;
        std::uint32_t high = count;
        while (high - low > 1U) {
            const std::uint32_t middle = low + ((high - low) / 2U);
            if (cdf[middle] <= u) {
                low = middle;
            } else {
                high = middle;
            }
        }
        return low;
    }

    /**
     * Where a tent over [-half, half], peaking at zero, reaches u in [0, 1): the inverse of its
     * normalised integral.
     **/
    MOYA_KERNEL static float sampleTent(float u, float half) {
        if (u < 0.5F) {
            return half * (std::sqrt(2.0F * u) - 1.0F);
        }
        return half * (1.0F - std::sqrt(2.0F - (2.0F * u)));
    }

    /** The offset and weight for two random numbers in [0, 1). **/
    MOYA_KERNEL FilterSample sample(glm::vec2 u) const {
        FilterSample out;
        switch (kind) {
        case FilterKind::Box:
            out.offset = ((2.0F * u) - 1.0F) * radius;
            out.weight = (2.0F * radius.x) * (2.0F * radius.y);
            break;
        case FilterKind::Triangle:
            out.offset = { sampleTent(u.x, radius.x), sampleTent(u.y, radius.y) };
            out.weight = radius.x * radius.y;
            break;
        case FilterKind::Table: {
            const std::uint32_t row = findInterval(&rowCdf[0], rows, u.y);
            const float down = (u.y - rowCdf[row]) / (rowCdf[row + 1U] - rowCdf[row]);
            const float * const cells = &cellCdf[row * (columns + 1U)];
            const std::uint32_t column = findInterval(cells, columns, u.x);
            const float across = (u.x - cells[column]) / (cells[column + 1U] - cells[column]);
            const glm::vec2 cell { (static_cast<float>(column) + across) / static_cast<float>(columns),
                (static_cast<float>(row) + down) / static_cast<float>(rows) };
            out.offset = ((2.0F * cell) - 1.0F) * radius;
            out.weight = values[(row * columns) + column] < 0.0F ? -integral : integral;
            break;
        }
        }
        return out;
    }
};

};  // namespace v3d::moya::kernel::film
