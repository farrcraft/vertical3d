/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/film/Filter.h>

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/kernel/film/PixelFilter.h>
#include <api/render/offline/film/Sampling.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

namespace v3d::moya::film {

namespace {

namespace offline = v3d::render::offline::film;

/** The cells a table has along an axis of the given width, in pixels. **/
std::uint32_t cells(float width) {
    const float wanted = std::ceil(width * static_cast<float>(Filter::kPerPixel));
    const float count = std::clamp(wanted, 1.0F, static_cast<float>(Filter::kMostCells));
    return static_cast<std::uint32_t>(count);  // checked: finite, and clamped to 1 to kMostCells
}

/** A buffer on the device holding the values. **/
boost::shared_ptr<device::Buffer> uploaded(device::Device & device, const std::vector<float> & values) {
    const std::size_t bytes = values.size() * sizeof(float);
    auto buffer = device.allocate(bytes);
    device.upload(*buffer, values.data(), bytes);
    return buffer;
}

/**
 * Divides a run of running sums, which starts at zero, by its last, so that it rises to one. A run
 * that sums to nothing rises evenly instead, since no sample is drawn from it.
 **/
void normalise(float * cdf, std::uint32_t count) {
    const float total = cdf[count];
    for (std::uint32_t i = 1; i <= count; ++i) {
        cdf[i] = total > 0.0F ? cdf[i] / total : static_cast<float>(i) / static_cast<float>(count);
    }
    cdf[count] = 1.0F;
}

};  // namespace

Filter::Filter(device::Device & device, offline::Filter kind, const glm::vec2 & width) {
    if (!offline::filterWidth(width)) {
        throw std::invalid_argument("a pixel filter's width must be positive and finite");
    }
    filter_.radius = width * 0.5F;
    if (kind == offline::Filter::Box) {
        filter_.kind = kernel::film::FilterKind::Box;
        return;
    }
    if (kind == offline::Filter::Triangle) {
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
            const float value = offline::filter(kind, centre - filter_.radius, width);
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

};  // namespace v3d::moya::film
