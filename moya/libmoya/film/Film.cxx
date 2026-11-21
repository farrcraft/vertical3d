/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/film/Film.h>

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/film/State.h>
#include <moya/libmoya/kernel/film/Combine.h>
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
        stride_ += kernel::film::floatsOf(channel.combine, channel.components);
    }

    // the moments and the weight follow the channels
    const std::size_t count = static_cast<std::size_t>(resolution.x) * resolution.y;
    pixels_ = device_.allocate(count * stored() * sizeof(float));
    stopped_ = device_.allocate(count * sizeof(std::uint32_t));
    clear();
}

void Film::clear() {
    const std::size_t count = static_cast<std::size_t>(resolution_.x) * resolution_.y;
    const std::size_t floats = count * stored();
    const std::vector<float> empty = emptyPixel();
    std::vector<float> start(floats);
    for (std::size_t i = 0; i < floats; i += empty.size()) {
        std::copy(empty.begin(), empty.end(), start.begin() + static_cast<std::ptrdiff_t>(i));
    }
    const std::scoped_lock lock(mutex_);
    device_.upload(*pixels_, start.data(), floats * sizeof(float));
    stops_.assign(count, 0U);
    device_.upload(*stopped_, stops_.data(), stops_.size() * sizeof(std::uint32_t));
    passes_ = 0;
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

kernel::film::Pixels Film::pixels() const {
    return kernel::film::Pixels { .values = device::view<float>(*pixels_),
        .width = resolution_.x,
        .stride = stored(),
        .stopped = device::view<std::uint32_t>(*stopped_) };
}

void Film::pass(const std::function<void(const kernel::film::Pixels &)> & launch) {
    const std::scoped_lock lock(mutex_);
    launch(pixels());
    ++passes_;
}

bool Film::attempt(const std::function<bool(const kernel::film::Pixels &)> & launch) {
    const std::scoped_lock lock(mutex_);
    before_.resize(static_cast<std::size_t>(resolution_.x) * resolution_.y * stored());
    device_.download(*pixels_, before_.data(), before_.size() * sizeof(float));
    if (!launch(pixels())) {
        device_.upload(*pixels_, before_.data(), before_.size() * sizeof(float));
        return false;
    }
    ++passes_;
    return true;
}

std::uint32_t Film::converge(float threshold) {
    const std::scoped_lock lock(mutex_);
    const std::size_t pixels = stops_.size();
    const std::uint32_t stored = this->stored();
    std::vector<float> sums(pixels * stored);
    device_.download(*pixels_, sums.data(), sums.size() * sizeof(float));
    std::vector<float> noises(pixels);
    for (std::size_t pixel = 0; pixel < pixels; ++pixel) {
        noises[pixel] = noise(&sums[pixel * stored], stride_);
    }

    // a pixel stops with its neighbours, so one whose halves agree by chance keeps sampling
    const auto quiet = [&](std::uint32_t x, std::uint32_t y) {
        return noises[(static_cast<std::size_t>(y) * resolution_.x) + x] < threshold;
    };
    std::uint32_t sampling = 0;
    for (std::uint32_t y = 0; y < resolution_.y; ++y) {
        for (std::uint32_t x = 0; x < resolution_.x; ++x) {
            std::uint32_t & stop = stops_[(static_cast<std::size_t>(y) * resolution_.x) + x];
            if (stop == 0U && quiet(x, y) && (x == 0 || quiet(x - 1U, y)) && (x + 1U == resolution_.x || quiet(x + 1U, y)) &&
                (y == 0 || quiet(x, y - 1U)) && (y + 1U == resolution_.y || quiet(x, y + 1U))) {
                stop = 1U;
            }
            sampling += stop == 0U ? 1U : 0U;
        }
    }
    device_.upload(*stopped_, stops_.data(), stops_.size() * sizeof(std::uint32_t));
    return sampling;
}

float Film::noise(const float * sums, std::uint32_t stride) {
    const float * const tail = sums + stride;
    const float weight = tail[kernel::film::kTailFloats - 1U];
    const float half = tail[kernel::film::kBeautyComponents];
    if (weight == 0.0F || half == 0.0F) {
        return std::numeric_limits<float>::infinity();
    }
    float apart = 0.0F;
    float total = 0.0F;
    for (std::uint32_t i = 0; i < kernel::film::kBeautyComponents; ++i) {
        const float mean = sums[i] / weight;
        apart += std::abs(mean - (tail[i] / half));
        total += mean;
    }
    // the small term keeps a black pixel's noise finite
    constexpr float kFloor = 1.0e-4F;
    return apart / (kFloor + std::sqrt(std::max(total, 0.0F)));
}

Snapshot Film::snapshot() const {
    const std::size_t pixels = static_cast<std::size_t>(resolution_.x) * resolution_.y;
    const std::uint32_t stored = this->stored();
    std::vector<float> sums(pixels * stored);
    Snapshot out { .resolution = resolution_, .passes = 0, .stride = stride_, .channels = channels_, .offsets = offsets_,
        .values = std::vector<float>(pixels * stride_), .errors = std::vector<float>(pixels), .samples = std::vector<std::uint32_t>(pixels) };
    {
        const std::scoped_lock lock(mutex_);
        device_.download(*pixels_, sums.data(), sums.size() * sizeof(float));
        out.passes = passes_;
    }
    for (std::size_t pixel = 0; pixel < pixels; ++pixel) {
        const float * const from = &sums[pixel * stored];
        const float weight = from[stored - 1U];
        float * const to = &out.values[pixel * stride_];
        out.samples[pixel] = static_cast<std::uint32_t>(from[stride_ + kernel::film::kHalfFloats]);
        for (std::uint32_t i = 0; i < kernel::film::kBeautyFloats; ++i) {
            to[i] = weight != 0.0F ? from[i] / weight : 0.0F;
        }
        resolveChannels(from, to, weight, out.samples[pixel] != 0U);
        const float * const moments = from + (stored - 1U - kernel::film::kMomentFloats);
        out.errors[pixel] = standardError(weight, moments, kernel::spectrum::acescgToXyz(glm::vec3(to[0], to[1], to[2])).y);
    }
    return out;
}

State Film::state() const {
    const std::scoped_lock lock(mutex_);
    State out { .resolution = resolution_, .passes = passes_, .channels = channels_, .stored = stored(),
        .sums = std::vector<float>(stops_.size() * stored()), .stopped = stops_ };
    device_.download(*pixels_, out.sums.data(), out.sums.size() * sizeof(float));
    return out;
}

void Film::restore(const State & state) {
    if (state.resolution != resolution_) {
        throw std::invalid_argument("a film state of another resolution");
    }
    bool same = state.channels.size() == channels_.size();
    for (std::size_t i = 0; same && i < channels_.size(); ++i) {
        same = state.channels[i].name == channels_[i].name && state.channels[i].components == channels_[i].components &&
               state.channels[i].combine == channels_[i].combine;
    }
    if (!same) {
        throw std::invalid_argument("a film state of other channels");
    }
    if (state.stored != stored() || state.sums.size() != stops_.size() * stored() || state.stopped.size() != stops_.size()) {
        throw std::invalid_argument("a film state whose floats or flags do not fill its resolution");
    }
    const std::scoped_lock lock(mutex_);
    device_.upload(*pixels_, state.sums.data(), state.sums.size() * sizeof(float));
    stops_ = state.stopped;
    device_.upload(*stopped_, stops_.data(), stops_.size() * sizeof(std::uint32_t));
    passes_ = state.passes;
}

void Film::resolveChannels(const float * from, float * to, float weight, bool reached) const {
    for (std::size_t i = 0; i < channels_.size(); ++i) {
        const Channel & channel = channels_[i];
        const bool mean = channel.combine == kernel::film::Combine::Mean;
        for (std::uint32_t j = offsets_[i]; j < offsets_[i] + kernel::film::floatsOf(channel.combine, channel.components); ++j) {
            if (mean) {
                to[j] = weight != 0.0F ? from[j] / weight : 0.0F;
            } else {
                to[j] = reached ? from[j] : 0.0F;
            }
        }
    }
}

std::vector<float> Film::emptyPixel() const {
    std::vector<float> out(stored(), 0.0F);
    for (std::size_t i = 0; i < channels_.size(); ++i) {
        const Channel & channel = channels_[i];
        float * const first = &out[offsets_[i]];
        switch (channel.combine) {
        case kernel::film::Combine::Mean:
        case kernel::film::Combine::Sum:
            break;
        case kernel::film::Combine::Closest:
            first[channel.components] = std::numeric_limits<float>::quiet_NaN();
            break;
        case kernel::film::Combine::Least:
            std::fill(first, first + channel.components, std::numeric_limits<float>::infinity());
            break;
        case kernel::film::Combine::Most:
            std::fill(first, first + channel.components, -std::numeric_limits<float>::infinity());
            break;
        }
    }
    return out;
}

std::uint32_t Film::stored() const {
    return stride_ + kernel::film::kTailFloats;
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
