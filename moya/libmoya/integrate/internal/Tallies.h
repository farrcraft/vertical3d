/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Tally.h>
#include <moya/libmoya/integrate/Profile.h>
#include <moya/libmoya/kernel/Array.h>

#include <cstddef>

#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

namespace v3d::moya::integrate::internal {

/**
 * A tally for each pixel of a counted render, row by row, in the device's memory, each zero to
 * begin. The device outlives it.
 **/
class Tallies final {
 public:
    Tallies(device::CpuDevice & device, const glm::uvec2 & resolution);

    /** The tallies as the profiled entry point takes them. **/
    kernel::Array<device::cpu::Tally> view() const;

    /**
     * Adds every pixel's rays to a profile, and gives it the share of every pixel's ticks that
     * traversal took, or zero when no pixel took any.
     **/
    void into(Profile & profile) const;

 private:
    device::CpuDevice & device_;
    std::size_t count_;
    boost::shared_ptr<device::Buffer> buffer_;
};

};  // namespace v3d::moya::integrate::internal
