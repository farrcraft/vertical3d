/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::geometry {

/**
 * A scene's prototypes in a device's memory as shading reads them: every geometry in its own space
 * at each sample, the instances that place them with their keys, and the primvars shading reads.
 * A colour primvar is converted into ACEScg from the space it names.
 *
 * It is held apart from any device's traversal, so a hit from any traversal of the same
 * prototypes is shaded alike. It reads the device's memory, so the device outlives it.
 **/
class Surfaces final {
 public:
    /**
     * @throws std::runtime_error, from OpenColorIO, when a colour primvar names a space the
     *         configuration cannot convert into ACEScg
     **/
    Surfaces(device::Device & device, const Prototypes & prototypes, const colour::Config & config);

    /** The surfaces, as kernel code reads them. **/
    kernel::surface::Surfaces view() const;

 private:
    boost::shared_ptr<device::Buffer> points_;
    boost::shared_ptr<device::Buffer> widths_;
    boost::shared_ptr<device::Buffer> quadrics_;
    boost::shared_ptr<device::Buffer> controls_;
    boost::shared_ptr<device::Buffer> ends_;
    boost::shared_ptr<device::Buffer> times_;
    boost::shared_ptr<device::Buffer> faces_;
    boost::shared_ptr<device::Buffer> geometries_;
    boost::shared_ptr<device::Buffer> keys_;
    boost::shared_ptr<device::Buffer> instances_;
    boost::shared_ptr<device::Buffer> byId_;
    boost::shared_ptr<device::Buffer> primvars_;
    boost::shared_ptr<device::Buffer> values_;
    float open_ { 0.0F };
    float close_ { 0.0F };
};

};  // namespace v3d::moya::geometry
