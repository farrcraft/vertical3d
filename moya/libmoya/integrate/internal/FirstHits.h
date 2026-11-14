/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>
#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/integrate/FirstHit.h>
#include <moya/libmoya/output/Channel.h>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::integrate::internal {

/**
 * The channels the path tracer fills at each camera ray's first hit, in a device's memory. The
 * device outlives them.
 *
 * A raw channel fills when output::readingNamed() knows its source name and its components are
 * the value's: one for a depth, an instance or a face, two for `st`, and three for a position, a
 * normal, or a denoiser's albedo or normal. A primvar channel fills from the primvar of its
 * source name, which the gather reads for it. Each other channel is reported, and holds zero.
 **/
class FirstHits final {
 public:
    /**
     * @param primvarNames the name of each primvar shading reads, by its number, as
     *        geometry::Prototypes gives them
     **/
    FirstHits(device::Device & device, const std::vector<output::Channel> & channels, const film::Film & film,
        const std::vector<std::string> & primvarNames, const boost::shared_ptr<log::Logger> & logger);

    /** The channels the path tracer fills at the first hit, as it takes them. **/
    kernel::Array<kernel::integrate::FirstHit> view() const;

    /** The channels of a denoiser's albedo and normal, as the path tracer takes them. **/
    kernel::Array<kernel::integrate::FirstHit> features() const;

 private:
    boost::shared_ptr<device::Buffer> buffer_;
    boost::shared_ptr<device::Buffer> features_;
};

};  // namespace v3d::moya::integrate::internal
