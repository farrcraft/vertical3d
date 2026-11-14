/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/integrate/internal/FirstHits.h>

#include <api/log/Logger.h>
#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/integrate/FirstHit.h>
#include <moya/libmoya/output/Channel.h>
#include <moya/libmoya/output/Outputs.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::integrate::internal {

namespace {

/** The components of the value a camera ray reads. **/
std::uint32_t componentsOf(kernel::integrate::Reading reading) {
    switch (reading) {
    case kernel::integrate::Reading::Position:
    case kernel::integrate::Reading::Normal:
        return 3U;
    case kernel::integrate::Reading::Texture:
        return 2U;
    case kernel::integrate::Reading::Depth:
    case kernel::integrate::Reading::Instance:
    case kernel::integrate::Reading::Face:
    case kernel::integrate::Reading::Primvar:
        break;
    }
    return 1U;
}

/** What a channel reads at the first hit, or none for a channel the path tracer does not fill. **/
std::optional<kernel::integrate::FirstHit> firstHitOf(const output::Channel & channel, const film::Film & film, const std::vector<std::string> & primvarNames) {
    kernel::integrate::FirstHit out { .offset = film.offset(channel.film.name),
        .components = channel.film.components,
        .primvar = 0,
        .reading = kernel::integrate::Reading::Primvar,
        .combine = channel.film.combine };
    if (channel.source == output::Source::Primvar) {
        const auto found = std::ranges::find(primvarNames, channel.sourceName);
        if (channel.sourceName.empty() || found == primvarNames.end()) {
            return std::nullopt;
        }
        out.primvar = static_cast<std::uint32_t>(found - primvarNames.begin());
        return out;
    }
    const std::optional<kernel::integrate::Reading> reading = channel.source == output::Source::Raw ? output::readingNamed(channel.sourceName) : std::nullopt;
    if (!reading || componentsOf(*reading) != channel.film.components) {
        return std::nullopt;
    }
    out.reading = *reading;
    return out;
}

};  // namespace

FirstHits::FirstHits(device::Device & device, const std::vector<output::Channel> & channels, const film::Film & film,
    const std::vector<std::string> & primvarNames, const boost::shared_ptr<log::Logger> & logger) {
    const std::array<const char *, 3> kinds { "raw value", "primvar", "light path expression" };
    std::vector<kernel::integrate::FirstHit> filled;
    for (const output::Channel & channel : channels) {
        const std::optional<kernel::integrate::FirstHit> read = firstHitOf(channel, film, primvarNames);
        if (read) {
            filled.push_back(*read);
        } else if (logger) {
            logger->get()->warn("channel {} of {} components collects the {} {}, which the path tracer does not fill, so it holds zero", channel.film.name,
                channel.film.components, kinds.at(static_cast<std::size_t>(channel.source)), channel.sourceName);
        }
    }
    const std::size_t bytes = filled.size() * sizeof(kernel::integrate::FirstHit);
    buffer_ = device.allocate(bytes);
    device.upload(*buffer_, filled.data(), bytes);
}

kernel::Array<kernel::integrate::FirstHit> FirstHits::view() const {
    return device::view<kernel::integrate::FirstHit>(*buffer_);
}

};  // namespace v3d::moya::integrate::internal
