/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/film/State.h>
#include <moya/libmoya/kernel/film/Pixels.h>

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

/**
 * moya's film: the pixels samples are added to, and the pixel filter that places them.
 **/
namespace v3d::moya::film {

/**
 * The pixels moya adds samples to, in a device's memory. Each pixel holds the beauty, in
 * ACEScg, and any number of named channels, each as weighted sums with one weight for the pixel.
 *
 * Samples are added in passes. A pass launches kernel code that adds each pixel's next samples to
 * that pixel, in index order. A snapshot can be taken from any thread at any time. It waits for a
 * pass in progress to end, so it holds every sample of the passes before it and none of the next.
 *
 * Between passes, converge() stops each pixel whose noise has fallen below a threshold, and a
 * pixel that has stopped takes no samples from the passes after it.
 *
 * The film reads its device's memory, so the device outlives it.
 **/
class Film final {
 public:
    /**
     * A film of the given resolution, every pixel zero, with the channels in the order given.
     *
     * @throws std::invalid_argument when a side is zero, when a channel has no name or the name
     *         of one before it, or when its components are not 1 to 4
     **/
    Film(device::Device & device, const glm::uvec2 & resolution, const std::vector<Channel> & channels);

    glm::uvec2 resolution() const;

    /**
     * Where a channel's first float is in a pixel, which kernel::film::Pixels::addChannel() and
     * Snapshot::at() take.
     *
     * @throws std::out_of_range when the film has no channel of that name
     **/
    std::uint32_t offset(const std::string & name) const;

    /**
     * Runs one pass. The function launches kernel code over the film's resolution with the pixels
     * it is given, and returns when every pixel has run. No snapshot reads the film meanwhile.
     **/
    void pass(const std::function<void(const kernel::film::Pixels &)> & launch);

    /**
     * Runs one pass that may stop part way. The function launches kernel code as pass() takes it,
     * and returns whether every pixel ran. A pass that stopped leaves every pixel's floats as they
     * stood before it, and is not counted, so a snapshot shows the passes before it alone.
     *
     * @return whether the pass ran to its end
     **/
    bool attempt(const std::function<bool(const kernel::film::Pixels &)> & launch);

    /** The film as it stood after the last pass to end. **/
    Snapshot snapshot() const;

    /** The film's floats and stopped pixels as they stood after the last pass to end. **/
    State state() const;

    /**
     * Replaces every pixel's floats, which pixels have stopped, and the count of passes with a
     * state's. The next pass continues from them.
     *
     * @throws std::invalid_argument when the state is of another resolution or other channels,
     *         or holds too few or too many floats or flags
     **/
    void restore(const State & state);

    /**
     * Stops each pixel whose noise and whose four neighbours' noise are all below a threshold.
     * A pixel's noise is noise() of its sums as the last pass left them. A pixel that has stopped
     * stays stopped.
     *
     * @return how many pixels still take samples
     **/
    std::uint32_t converge(float threshold);

    /**
     * A pixel's noise, from the floats it holds: how far its half beauty is from its beauty,
     * summed over red, green and blue, over the square root of the beauty's sum, as Cycles
     * measures it. It is infinite while either holds no weight.
     *
     * @param sums a pixel's floats as the device holds them
     * @param stride the floats the beauty and the channels take, before the half beauty
     **/
    static float noise(const float * sums, std::uint32_t stride);

 private:
    /**
     * A pixel no sample has reached. Its floats are zero, but for three. A least channel's
     * components are infinite, and a most channel's are minus infinity. A closest channel's key
     * is NaN.
     **/
    std::vector<float> emptyPixel() const;

    /**
     * A pixel's channels resolved from its sums: a mean's divided by the pixel's weight, and any
     * other kept as it is. Every float of a pixel no sample reached is zero.
     **/
    void resolveChannels(const float * from, float * to, float weight, bool reached) const;

    /** The floats each pixel holds on the device: the snapshot's, the moments and the weight. **/
    std::uint32_t stored() const;

    /** The pixels as kernel code takes them. **/
    kernel::film::Pixels pixels() const;

    /**
     * The standard error of a pixel's mean luminance, from its weight and its moments, or
     * infinity when its samples weigh less than two of equal weight.
     **/
    static float standardError(float weight, const float * moments, float mean);

    device::Device & device_;
    glm::uvec2 resolution_;
    std::vector<Channel> channels_;
    std::vector<std::uint32_t> offsets_;
    std::uint32_t stride_ { kernel::film::kBeautyFloats };
    boost::shared_ptr<device::Buffer> pixels_;
    std::uint32_t passes_ { 0 };

    /** The pixels' floats as they stood before the pass that attempt() runs. **/
    std::vector<float> before_;

    /** Each pixel's flag, which is not zero once it has stopped, on the device and on the host. **/
    boost::shared_ptr<device::Buffer> stopped_;
    std::vector<std::uint32_t> stops_;

    /** Held by a pass and by a snapshot, so that neither sees the other half done. **/
    mutable std::mutex mutex_;
};

};  // namespace v3d::moya::film
