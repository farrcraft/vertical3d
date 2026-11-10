/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/film/Film.h>

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya::film {

Film::Film(device::Device & device, const glm::uvec2 & resolution, const std::vector<Channel> & channels) :
    device_(device), resolution_(resolution), channels_(channels) {
    if (resolution.x == 0 || resolution.y == 0) {
        throw std::invalid_argument("a film with no pixels");
    }
    for (std::size_t i = 0; i < channels_.size(); ++i) {
        const Channel & channel = channels_[i];
        if (channel.name.empty()) {
            throw std::invalid_argument("a film channel with no name");
        }
        if (channel.components < 1 || channel.components > 4) {
            throw std::invalid_argument("film channel " + channel.name + " has components outside 1 to 4");
        }
        for (std::size_t before = 0; before < i; ++before) {
            if (channels_[before].name == channel.name) {
                throw std::invalid_argument("two film channels named " + channel.name);
            }
        }
        offsets_.push_back(stride_);
        stride_ += channel.components;
    }

    // the moments and the weight follow the channels
    const std::size_t floats = static_cast<std::size_t>(resolution.x) * resolution.y * stored();
    const std::vector<float> zeros(floats, 0.0F);
    pixels_ = device_.allocate(floats * sizeof(float));
    device_.upload(*pixels_, zeros.data(), floats * sizeof(float));
}

glm::uvec2 Film::resolution() const {
    return resolution_;
}

std::uint32_t Film::offset(const std::string & name) const {
    for (std::size_t i = 0; i < channels_.size(); ++i) {
        if (channels_[i].name == name) {
            return offsets_[i];
        }
    }
    throw std::out_of_range("the film has no channel named " + name);
}

void Film::pass(const std::function<void(const kernel::film::Pixels &)> & launch) {
    const std::scoped_lock lock(mutex_);
    launch(kernel::film::Pixels { device::view<float>(*pixels_), resolution_.x, stored() });
    ++passes_;
}

Snapshot Film::snapshot() const {
    const std::size_t pixels = static_cast<std::size_t>(resolution_.x) * resolution_.y;
    const std::uint32_t stored = this->stored();
    std::vector<float> sums(pixels * stored);
    Snapshot out { .resolution = resolution_, .passes = 0, .stride = stride_, .channels = channels_, .offsets = offsets_,
        .values = std::vector<float>(pixels * stride_), .errors = std::vector<float>(pixels) };
    {
        const std::scoped_lock lock(mutex_);
        device_.download(*pixels_, sums.data(), sums.size() * sizeof(float));
        out.passes = passes_;
    }
    for (std::size_t pixel = 0; pixel < pixels; ++pixel) {
        const float * const from = &sums[pixel * stored];
        const float weight = from[stored - 1U];
        float * const to = &out.values[pixel * stride_];
        for (std::uint32_t i = 0; i < stride_; ++i) {
            to[i] = weight != 0.0F ? from[i] / weight : 0.0F;
        }
        const float * const moments = from + stride_;
        out.errors[pixel] = standardError(weight, moments, kernel::spectrum::acescgToXyz(glm::vec3(to[0], to[1], to[2])).y);
    }
    return out;
}

std::uint32_t Film::stored() const {
    return stride_ + kernel::film::kMomentFloats + 1U;
}

float Film::standardError(float weight, const float * moments, float mean) {
    // the variance of a weighted mean of independent samples, scaled by n / (n - 1) for an
    // effective count n of the squared sum of weights over the sum of squared weights
    const double sum = weight;
    const double squares = moments[2];
    if (squares <= 0.0) {
        return std::numeric_limits<float>::infinity();
    }
    const double count = (sum * sum) / squares;
    if (count <= 1.0) {
        return std::numeric_limits<float>::infinity();
    }
    const double centre = mean;
    const double spread = std::max(0.0, moments[0] - (2.0 * centre * moments[1]) + (centre * centre * squares));
    return static_cast<float>(std::sqrt((spread / (sum * sum)) * (count / (count - 1.0))));
}

};  // namespace v3d::moya::film
