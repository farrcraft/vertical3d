/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/kernel/film/Pixels.h>

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

/**
 * The new core's film: the pixels samples are added to, and the pixel filter that places them.
 * The reyes and ray hiders use render::offline::film instead.
 **/
namespace v3d::moya::film {

/** A channel a film holds beside the beauty: an output such as albedo, normal or depth. **/
struct Channel {
    std::string name;

    /** Its floats per pixel, from 1 to 4. **/
    std::uint32_t components { 1 };
};

/**
 * A film's pixels as they stood after some number of passes, with each sum divided by its pixel's
 * weight. A pixel no sample reached is zero.
 **/
struct Snapshot {
    glm::uvec2 resolution { 0 };

    /** The passes it holds the samples of. **/
    std::uint32_t passes { 0 };

    /** The floats each pixel holds: the beauty's three, then each channel's, in order. **/
    std::uint32_t stride { 0 };

    /** The film's channels, in order, and where each one's first float sits in a pixel. **/
    std::vector<Channel> channels;
    std::vector<std::uint32_t> offsets;

    /** Every pixel's floats, row by row from the top left. **/
    std::vector<float> values;

    /** A pixel's float at an offset: 0 to 2 for the beauty, or offset() for a channel. **/
    float at(std::uint32_t x, std::uint32_t y, std::uint32_t offset) const;

    /**
     * Where a channel's first float sits in a pixel, as Film::offset() gives it.
     *
     * @throws std::out_of_range when the snapshot has no channel of that name
     **/
    std::uint32_t offset(const std::string & name) const;
};

/**
 * The pixels the new core adds samples to, in a device's memory. Each pixel holds the beauty, in
 * ACEScg, and any number of named channels, each as weighted sums with one weight for the pixel.
 *
 * Samples are added in passes. A pass launches kernel code that adds each pixel's next samples to
 * that pixel, in index order. A snapshot can be taken from any thread at any time. It waits for a
 * pass in progress to end, so it holds every sample of the passes before it and none of the next.
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

    /** The film as it stood after the last pass to end. **/
    Snapshot snapshot() const;

 private:
    device::Device & device_;
    glm::uvec2 resolution_;
    std::vector<Channel> channels_;
    std::vector<std::uint32_t> offsets_;
    std::uint32_t stride_ { kernel::film::kBeautyComponents };
    boost::shared_ptr<device::Buffer> pixels_;
    std::uint32_t passes_ { 0 };

    /** Held by a pass and by a snapshot, so that neither sees the other half done. **/
    mutable std::mutex mutex_;
};

};  // namespace v3d::moya::film
