/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/kernel/film/PixelFilter.h>
#include <api/render/offline/film/Sampling.h>

#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

namespace v3d::moya::film {

/**
 * A pixel filter RI names, on a device, for kernel code to draw camera samples from.
 *
 * A box and a triangle need nothing on the device. Every other filter is tabulated at kPerPixel
 * cells per pixel of its width along each axis, and at most kMostCells, with each cell's value
 * the filter's at the cell's centre.
 **/
class Filter final {
 public:
    /** The table's cells per pixel of the filter's width, along each axis. **/
    static constexpr std::uint32_t kPerPixel = 32;

    /** The most cells a table has along one axis, however wide the filter. **/
    static constexpr std::uint32_t kMostCells = 1024;

    /**
     * @param kind which of RI's filters
     * @param width the filter's whole extent along each axis, in pixels, as PixelFilter gives it
     * @throws std::invalid_argument when render::offline::film::filterWidth() refuses the width
     **/
    Filter(device::Device & device, render::offline::film::Filter kind, const glm::vec2 & width);

    /** The filter as kernel code reads it, valid while this filter lives. **/
    kernel::film::PixelFilter view() const;

 private:
    kernel::film::PixelFilter filter_;
    boost::shared_ptr<device::Buffer> values_;
    boost::shared_ptr<device::Buffer> rowCdf_;
    boost::shared_ptr<device::Buffer> cellCdf_;

    /** The cells a table has along an axis of the given width, in pixels. **/
    static std::uint32_t cells(float width);

    /** A buffer on the device holding the values. **/
    static boost::shared_ptr<device::Buffer> uploaded(device::Device & device, const std::vector<float> & values);

    /**
     * Divides a run of running sums, which starts at zero, by its last, so that it rises to one. A
     * run that sums to nothing rises evenly instead, since no sample is drawn from it.
     **/
    static void normalise(float * cdf, std::uint32_t count);
};

};  // namespace v3d::moya::film
