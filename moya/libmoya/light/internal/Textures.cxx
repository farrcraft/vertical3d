/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/light/internal/Textures.h>

#include <api/log/Logger.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/Transform.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/light/Image.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/output/Chromaticities.h>
#include <moya/libmoya/output/ExrImage.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

namespace v3d::moya::light::internal {

namespace {

constexpr double kPi = 3.14159265358979323846;

/** The luminance of an ACEScg colour, held to zero or more. **/
float luminanceOf(const glm::vec3 & colour) {
    return std::fmax(0.0F, kernel::spectrum::acescgToXyz(colour).y);
}

/**
 * The sums of a run of weights up to and including each, over their total, the last held to one.
 * Empty when the weights add up to nothing.
 **/
std::vector<float> summed(const float * weights, std::size_t count) {
    double total = 0.0;
    for (std::size_t i = 0; i < count; ++i) {
        total += weights[i];
    }
    std::vector<float> out;
    if (!(total > 0.0)) {
        return out;
    }
    double sum = 0.0;
    for (std::size_t i = 0; i < count; ++i) {
        sum += weights[i];
        out.push_back(static_cast<float>(sum / total));
    }
    out.back() = 1.0F;
    return out;
}

};  // namespace

Textures::Textures(const colour::Config & config, boost::shared_ptr<log::Logger> logger) : config_(config), logger_(std::move(logger)) {
}

void Textures::report(const std::string & message) {
    if (reported_.insert(message).second && logger_) {
        logger_->get()->warn("{}", message);
    }
}

std::uint32_t Textures::load(const std::string & path) {
    const auto held = byPath_.find(path);
    if (held != byPath_.end()) {
        return held->second;
    }
    output::ExrImage read;
    try {
        read = output::readExr(path);
    } catch (const std::exception & error) {
        report("the light's texture " + path + " cannot be read, so it is left out: " + error.what());
        byPath_.emplace(path, kernel::geometry::kNoPrimitive);
        return kernel::geometry::kNoPrimitive;
    }
    const std::string space = config_.spaceOf(read.interopId.empty() ? output::kLinearIds[output::kRec709] : read.interopId);
    if (read.interopId.empty()) {
        report("the light's texture " + path + " has chromaticities of no space moya knows, so it is read as linear Rec. 709");
    }
    kernel::light::Image image;
    image.texels = static_cast<std::uint32_t>(texels_.size());
    image.width = read.width;
    image.height = read.height;
    if (!read.texels.empty()) {
        config_.input(space).apply(&read.texels.front().x, read.texels.size(), 3);
    }
    texels_.insert(texels_.end(), read.texels.begin(), read.texels.end());
    const auto place = static_cast<std::uint32_t>(images_.size());
    images_.push_back(image);
    cubes_.push_back(read.cube);
    byPath_.emplace(path, place);
    return place;
}

bool Textures::cube(std::uint32_t image) const {
    return cubes_[image];
}

std::vector<float> Textures::texelAngles(const kernel::light::Image & image) {
    std::vector<float> out;
    for (std::uint32_t row = 0; row < image.height; ++row) {
        // a band of latitude covers 2 pi (sin top - sin bottom), split over its texels
        const double top = kPi * (0.5 - (static_cast<double>(row) / image.height));
        const double bottom = kPi * (0.5 - (static_cast<double>(row + 1U) / image.height));
        out.push_back(static_cast<float>(2.0 * kPi * (std::sin(top) - std::sin(bottom)) / image.width));
    }
    return out;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the rows' first sum, then the columns'
void Textures::appendWay(const kernel::light::Image & image, const std::vector<float> & weights, std::uint32_t & rows, std::uint32_t & columns) {
    std::vector<float> rowWeights;
    std::vector<float> columnSums;
    const std::vector<float> angles = texelAngles(image);
    for (std::uint32_t row = 0; row < image.height; ++row) {
        const float * along = &weights[static_cast<std::size_t>(row) * image.width];
        double total = 0.0;
        for (std::uint32_t column = 0; column < image.width; ++column) {
            total += along[column];
        }
        rowWeights.push_back(static_cast<float>(total));
        std::vector<float> sums = summed(along, image.width);
        if (sums.empty()) {
            // a row that gives nothing to draw by is drawn evenly along it
            const std::vector<float> even(image.width, 1.0F);
            sums = summed(even.data(), even.size());
        }
        columnSums.insert(columnSums.end(), sums.begin(), sums.end());
    }
    std::vector<float> rowSums = summed(rowWeights.data(), rowWeights.size());
    if (rowSums.empty()) {
        rowSums = summed(angles.data(), angles.size());
    }
    rows = static_cast<std::uint32_t>(sums_.size());
    sums_.insert(sums_.end(), rowSums.begin(), rowSums.end());
    columns = static_cast<std::uint32_t>(sums_.size());
    sums_.insert(sums_.end(), columnSums.begin(), columnSums.end());
}

void Textures::drawable(std::uint32_t image) {
    kernel::light::Image & held = images_[image];
    if (held.width == 0U || held.height == 0U) {
        return;
    }
    const std::vector<float> angles = texelAngles(held);
    std::vector<float> weights;
    weights.reserve(static_cast<std::size_t>(held.width) * held.height);
    for (std::uint32_t row = 0; row < held.height; ++row) {
        for (std::uint32_t column = 0; column < held.width; ++column) {
            weights.push_back(luminanceOf(texels_[held.texels + (row * held.width) + column]) * angles[row]);
        }
    }
    // compensation takes the average luminance over the sphere off every texel
    const float average = integral(image) / static_cast<float>(4.0 * kPi);
    std::vector<float> compensated;
    compensated.reserve(weights.size());
    for (std::uint32_t row = 0; row < held.height; ++row) {
        for (std::uint32_t column = 0; column < held.width; ++column) {
            const float luminance = luminanceOf(texels_[held.texels + (row * held.width) + column]);
            compensated.push_back(std::fmax(0.0F, luminance - average) * angles[row]);
        }
    }
    appendWay(held, weights, held.rows, held.columns);
    appendWay(held, compensated, held.compensatedRows, held.compensatedColumns);
}

float Textures::integral(std::uint32_t image) const {
    const kernel::light::Image & held = images_[image];
    const std::vector<float> angles = texelAngles(held);
    double sum = 0.0;
    for (std::uint32_t row = 0; row < held.height; ++row) {
        for (std::uint32_t column = 0; column < held.width; ++column) {
            sum += static_cast<double>(luminanceOf(texels_[held.texels + (row * held.width) + column])) * angles[row];
        }
    }
    return static_cast<float>(sum);
}

float Textures::average(std::uint32_t image) const {
    const kernel::light::Image & held = images_[image];
    const std::size_t count = static_cast<std::size_t>(held.width) * held.height;
    if (count == 0U) {
        return 0.0F;
    }
    double sum = 0.0;
    for (std::size_t i = 0; i < count; ++i) {
        sum += luminanceOf(texels_[held.texels + i]);
    }
    return static_cast<float>(sum / static_cast<double>(count));
}

const std::vector<kernel::light::Image> & Textures::images() const {
    return images_;
}

const std::vector<glm::vec3> & Textures::texels() const {
    return texels_;
}

const std::vector<float> & Textures::sums() const {
    return sums_;
}

};  // namespace v3d::moya::light::internal
