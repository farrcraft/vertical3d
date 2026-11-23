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

struct Grid;

/**
 * Every volume the prototypes place, and the grids their fields read, held on a device. Each
 * grid's NanoVDB buffer follows the one before, among one run of blocks.
 *
 * Volumes can be kept from one version of a scene to the next. An update lays the blocks out
 * again only when the prototypes' grids are not the ones laid out before, in the same order.
 **/
class Volumes final {
 public:
    Volumes(device::Device & device, const geometry::Prototypes & prototypes);

    /** Brings the volumes up to another gather's prototypes. **/
    void update(device::Device & device, const geometry::Prototypes & prototypes);

    /** The volumes, as kernel code reads them. **/
    kernel::volume::Volumes view() const;

    /** Whether the last update, or the construction, laid out the grids' blocks. **/
    bool laidOut() const {
        return laidOut_;
    }

 private:
    /** A buffer of the device's that holds a copy of values. **/
    template <typename T>
    static boost::shared_ptr<device::Buffer> uploaded(device::Device & device, const std::vector<T> & values);

    boost::shared_ptr<device::Buffer> volumes_;
    boost::shared_ptr<device::Buffer> grids_;
    boost::shared_ptr<device::Buffer> blocks_;

    /** The grids the blocks were laid out from, in order. **/
    std::vector<boost::shared_ptr<const Grid>> laid_;
    bool laidOut_ { false };
};

};  // namespace v3d::moya::volume
