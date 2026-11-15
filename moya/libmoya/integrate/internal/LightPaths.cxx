/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/integrate/internal/LightPaths.h>

#include <api/log/Logger.h>
#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/kernel/integrate/Expressions.h>
#include <moya/libmoya/lpe/Compile.h>
#include <moya/libmoya/lpe/Refused.h>
#include <moya/libmoya/output/Channel.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::integrate::internal {

namespace {

/** The components of a channel a light path expression fills: ACEScg's red, green and blue. **/
constexpr std::uint32_t kComponents = 3;

/** Values in the device's memory. **/
template <typename T>
boost::shared_ptr<device::Buffer> uploaded(device::Device & device, const std::vector<T> & values) {
    const std::size_t bytes = values.size() * sizeof(T);
    boost::shared_ptr<device::Buffer> out = device.allocate(bytes);
    device.upload(*out, values.data(), bytes);
    return out;
}

};  // namespace

LightPaths::LightPaths(device::Device & device, const std::vector<output::Channel> & channels, const film::Film & film,
    const std::vector<std::string> & labels, const boost::shared_ptr<log::Logger> & logger) {
    std::vector<std::string> expressions;
    std::vector<kernel::integrate::Expressions::Channel> filled;
    for (const output::Channel & channel : channels) {
        if (channel.source != output::Source::Lpe) {
            continue;
        }
        const bool fits = channel.film.components == kComponents && filled.size() < kernel::integrate::kMostExpressions;
        if (!fits) {
            if (logger) {
                logger->get()->warn("channel {} of {} components collects the light path expression {}, which the path tracer fills only for one of {} "
                                    "components among the first {}, so it holds zero",
                    channel.film.name, channel.film.components, channel.sourceName, kComponents, kernel::integrate::kMostExpressions);
            }
            continue;
        }
        expressions.push_back(channel.sourceName);
        filled.push_back(kernel::integrate::Expressions::Channel { .offset = film.offset(channel.film.name), .combine = channel.film.combine });
    }
    std::vector<std::uint32_t> next;
    std::vector<std::uint32_t> accepts;
    if (!filled.empty()) {
        try {
            const lpe::Compiled compiled = lpe::compile(expressions, labels);
            for (const lpe::Compiled::Unread & unread : compiled.unread) {
                if (logger) {
                    logger->get()->warn("the light path expression {} cannot be read at {}: {}, so its channel holds zero", expressions[unread.expression],
                        unread.failure.place, unread.failure.reason);
                }
            }
            next = compiled.automaton.next;
            symbols_ = compiled.automaton.symbols;
            for (const std::vector<std::uint32_t> & accepted : compiled.automaton.accepts) {
                std::uint32_t flags = 0;
                for (const std::uint32_t expression : accepted) {
                    flags |= 1U << expression;
                }
                accepts.push_back(flags);
            }
        } catch (const lpe::Refused & refused) {
            if (logger) {
                logger->get()->error("{}, so every channel of a light path expression holds zero", refused.what());
            }
            filled.clear();
        }
    }
    next_ = uploaded(device, next);
    accepts_ = uploaded(device, accepts);
    channels_ = uploaded(device, filled);
}

kernel::integrate::Expressions LightPaths::view() const {
    return kernel::integrate::Expressions { .next = device::view<std::uint32_t>(*next_),
        .accepts = device::view<std::uint32_t>(*accepts_),
        .channels = device::view<kernel::integrate::Expressions::Channel>(*channels_),
        .symbols = symbols_ };
}

};  // namespace v3d::moya::integrate::internal
