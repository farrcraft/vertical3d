/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/film/Filter.h>

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/kernel/film/PixelFilter.h>
#include <api/rib/Sampling.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

namespace v3d::moya::film {

namespace {

/** The coefficients of a cubic in the radius, of radius^3, radius^2, radius and 1 in turn. **/
typedef std::array<float, 4> Cubic;

/**
 * The two pieces of the Catmull-Rom kernel, doubled: one for a radius under one, and one for a
 * radius from one to two. Doubling scales every weight alike, and the film divides by the total
 * weight.
 **/
constexpr Cubic CATMULL_ROM_INNER{3.0F, -5.0F, 0.0F, 2.0F};
constexpr Cubic CATMULL_ROM_OUTER{-1.0F, 5.0F, -8.0F, 4.0F};

/** A cubic at a radius, given the radius and its square. **/
float evaluate(const Cubic & cubic, float radius, float radiusSquared) {
    return (cubic[0] * radius * radiusSquared) + (cubic[1] * radiusSquared) + (cubic[2] * radius) + cubic[3];
}

};  // namespace

Filter::Filter(device::Device & device, rib::Filter kind, const glm::vec2 & width) {
    if (!rib::filterWidth(width)) {
        throw std::invalid_argument("a pixel filter's width must be positive and finite");
    }
    filter_.radius = width * 0.5F;
    if (kind == rib::Filter::Box) {
        filter_.kind = kernel::film::FilterKind::Box;
        return;
    }
    if (kind == rib::Filter::Triangle) {
        filter_.kind = kernel::film::FilterKind::Triangle;
        return;
    }

    filter_.kind = kernel::film::FilterKind::Table;
    filter_.columns = cells(width.x);
    filter_.rows = cells(width.y);
    const std::uint32_t columns = filter_.columns;
    const std::uint32_t rows = filter_.rows;
    const glm::vec2 cell { width.x / static_cast<float>(columns), width.y / static_cast<float>(rows) };

    std::vector<float> values(static_cast<std::size_t>(rows) * columns);
    std::vector<float> rowCdf(rows + 1U, 0.0F);
    std::vector<float> cellCdf(static_cast<std::size_t>(rows) * (columns + 1U), 0.0F);
    double total = 0.0;
    for (std::uint32_t row = 0; row < rows; ++row) {
        float * const cdf = &cellCdf[static_cast<std::size_t>(row) * (columns + 1U)];
        double sum = 0.0;
        for (std::uint32_t column = 0; column < columns; ++column) {
            const glm::vec2 centre { (static_cast<float>(column) + 0.5F) * cell.x, (static_cast<float>(row) + 0.5F) * cell.y };
            const float value = weight(kind, centre - filter_.radius, width);
            values[(static_cast<std::size_t>(row) * columns) + column] = value;
            sum += std::fabs(value);
            cdf[column + 1U] = static_cast<float>(sum);
        }
        normalise(cdf, columns);
        total += sum;
        rowCdf[row + 1U] = static_cast<float>(total);
    }
    if (!(total > 0.0)) {
        throw std::invalid_argument("a pixel filter that is zero over its whole width");
    }
    normalise(rowCdf.data(), rows);
    filter_.integral = static_cast<float>(total) * cell.x * cell.y;

    values_ = uploaded(device, values);
    rowCdf_ = uploaded(device, rowCdf);
    cellCdf_ = uploaded(device, cellCdf);
    filter_.values = device::view<float>(*values_);
    filter_.rowCdf = device::view<float>(*rowCdf_);
    filter_.cellCdf = device::view<float>(*cellCdf_);
}

kernel::film::PixelFilter Filter::view() const {
    return filter_;
}

std::uint32_t Filter::cells(float width) {
    const float wanted = std::ceil(width * static_cast<float>(kPerPixel));
    const float count = std::clamp(wanted, 1.0F, static_cast<float>(kMostCells));
    return static_cast<std::uint32_t>(count);  // checked: finite, and clamped to 1 to kMostCells
}

boost::shared_ptr<device::Buffer> Filter::uploaded(device::Device & device, const std::vector<float> & values) {
    const std::size_t bytes = values.size() * sizeof(float);
    auto buffer = device.allocate(bytes);
    device.upload(*buffer, values.data(), bytes);
    return buffer;
}

void Filter::normalise(float * cdf, std::uint32_t count) {
    const float total = cdf[count];
    for (std::uint32_t i = 1; i <= count; ++i) {
        cdf[i] = total > 0.0F ? cdf[i] / total : static_cast<float>(i) / static_cast<float>(count);
    }
    cdf[count] = 1.0F;
}

float Filter::weight(rib::Filter kind, const glm::vec2 & offset, const glm::vec2 & width) {
    const glm::vec2 half = width * 0.5F;
    if (!(std::fabs(offset.x) <= half.x) || !(std::fabs(offset.y) <= half.y)) {
        return 0.0F;
    }
    switch (kind) {
    case rib::Filter::Box:
        return 1.0F;
    case rib::Filter::Triangle:
        return (1.0F - (std::fabs(offset.x) / half.x)) * (1.0F - (std::fabs(offset.y) / half.y));
    case rib::Filter::CatmullRom: {
        // radial and not scaled by the width. The kernel reaches zero two pixels out, so a
        // width under four cuts it off at the width test above. RI applies that cut-off to
        // every filter
        const float radiusSquared = (offset.x * offset.x) + (offset.y * offset.y);
        const float radius = std::sqrt(radiusSquared);
        if (radius >= 2.0F) {
            return 0.0F;
        }
        if (radius < 1.0F) {
            return evaluate(CATMULL_ROM_INNER, radius, radiusSquared);
        }
        return evaluate(CATMULL_ROM_OUTER, radius, radiusSquared);
    }
    case rib::Filter::Gaussian: {
        // scaled so that the edge of the width is two standard deviations out
        const float x = offset.x * 2.0F / width.x;
        const float y = offset.y * 2.0F / width.y;
        return std::exp(-2.0F * ((x * x) + (y * y)));
    }
    case rib::Filter::Sinc: {
        const float pi = 3.14159265358979F;
        const float x = offset.x == 0.0F ? 1.0F : std::sin(pi * offset.x) / (pi * offset.x);
        const float y = offset.y == 0.0F ? 1.0F : std::sin(pi * offset.y) / (pi * offset.y);
        return x * y;
    }
    }
    return 0.0F;
}

};  // namespace v3d::moya::film
