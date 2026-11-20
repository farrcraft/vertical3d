/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/volume/Volumes.h>

#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::device {
class Buffer;
class Device;
};  // namespace v3d::moya::device

namespace v3d::moya::geometry {
struct Prototypes;
};  // namespace v3d::moya::geometry

namespace v3d::moya::volume {

/**
 * Every volume the prototypes place, and the grids their fields read, held on a device. Each
 * grid's NanoVDB buffer follows the one before, among one run of blocks.
 **/
class Volumes final {
 public:
    Volumes(device::Device & device, const geometry::Prototypes & prototypes);

    /** The volumes, as kernel code reads them. **/
    kernel::volume::Volumes view() const;

 private:
    /** A buffer of the device's that holds a copy of values. **/
    template <typename T>
    static boost::shared_ptr<device::Buffer> uploaded(device::Device & device, const std::vector<T> & values);

    boost::shared_ptr<device::Buffer> volumes_;
    boost::shared_ptr<device::Buffer> grids_;
    boost::shared_ptr<device::Buffer> blocks_;
};

};  // namespace v3d::moya::volume
