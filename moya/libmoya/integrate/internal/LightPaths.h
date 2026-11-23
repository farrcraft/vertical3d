/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>
#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/kernel/integrate/Expressions.h>
#include <moya/libmoya/output/Channel.h>

#include <cstdint>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::integrate::internal {

/**
 * The channels the path tracer fills from light path expressions, compiled into one automaton
 * in a device's memory. The device outlives them.
 *
 * Each channel whose source is a light path expression, of three components, takes the light of
 * the paths its expression matches, in ACEScg. Its expression is read under the render's light
 * groups. A channel of other components, one past the first kernel::integrate::kMostExpressions,
 * and one whose expression cannot be read are reported, and hold zero. A set of expressions whose
 * automaton is refused is reported, and every one of its channels holds zero.
 **/
class LightPaths final {
 public:
    /** @param labels the names of the render's light groups, by the label each light carries **/
    LightPaths(device::Device & device, const std::vector<output::Channel> & channels, const film::Film & film, const std::vector<std::string> & labels,
        const boost::shared_ptr<log::Logger> & logger);

    /** The expressions as the path tracer takes them, which hold no channel when none fills. **/
    kernel::integrate::Expressions view() const;

 private:
    boost::shared_ptr<device::Buffer> next_;
    boost::shared_ptr<device::Buffer> accepts_;
    boost::shared_ptr<device::Buffer> channels_;
    std::uint32_t symbols_ { 0 };
};

};  // namespace v3d::moya::integrate::internal
