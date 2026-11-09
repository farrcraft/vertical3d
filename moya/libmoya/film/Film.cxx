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

#include <cstddef>
#include <cstdint>
#include <functional>
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

    // the weight follows the channels
    const std::size_t floats = static_cast<std::size_t>(resolution.x) * resolution.y * (stride_ + 1U);
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
    launch(kernel::film::Pixels { device::view<float>(*pixels_), resolution_.x, stride_ + 1U });
    ++passes_;
}

Snapshot Film::snapshot() const {
    const std::size_t pixels = static_cast<std::size_t>(resolution_.x) * resolution_.y;
    const std::uint32_t stored = stride_ + 1U;
    std::vector<float> sums(pixels * stored);
    Snapshot out { .resolution = resolution_, .passes = 0, .stride = stride_, .channels = channels_, .offsets = offsets_,
        .values = std::vector<float>(pixels * stride_) };
    {
        const std::scoped_lock lock(mutex_);
        device_.download(*pixels_, sums.data(), sums.size() * sizeof(float));
        out.passes = passes_;
    }
    for (std::size_t pixel = 0; pixel < pixels; ++pixel) {
        const float * const from = &sums[pixel * stored];
        const float weight = from[stride_];
        float * const to = &out.values[pixel * stride_];
        for (std::uint32_t i = 0; i < stride_; ++i) {
            to[i] = weight != 0.0F ? from[i] / weight : 0.0F;
        }
    }
    return out;
}

};  // namespace v3d::moya::film
